import json
import math
import random
import socket
import sys
import time
from PyQt6.QtCore import QTimer, Qt
from PyQt6.QtWidgets import (
    QApplication,
    QCheckBox,
    QComboBox,
    QFrame,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QStackedWidget,
    QVBoxLayout,
    QWidget,
)
import pyqtgraph as pg

UDP_IP = "0.0.0.0"
UDP_PORT_RX = 5005
VEHICLE_IP = "192.168.1.109"
VEHICLE_PORT_TX = 5006

STYLE_KEY_RELEASED = """
    QLabel {
        background-color: #2A2A2A;
        color: #777777;
        border: 2px solid #444444;
        border-radius: 10px;
        font-size: 24px;
        font-weight: bold;
    }
"""

STYLE_KEY_PRESSED = """
    QLabel {
        background-color: #00E676;
        color: #121212;
        border: 2px solid #00E676;
        border-radius: 10px;
        font-size: 26px;
        font-weight: bold;
    }
"""

DARK_STYLESHEET = """
    QMainWindow {
        background-color: #121212;
    }
    QWidget {
        color: #E0E0E0;
        font-family: 'Segoe UI', Arial, sans-serif;
        font-size: 13px;
    }
    QComboBox {
        background-color: #2A2A2A;
        border: 1px solid #444444;
        border-radius: 4px;
        padding: 5px 10px;
        color: #FFFFFF;
        min-width: 220px;
    }
    QCheckBox {
        font-weight: bold;
        color: #00E676;
    }
    #statusLabel {
        font-weight: bold;
        padding: 5px 12px;
        border-radius: 6px;
        font-size: 13px;
    }
    #cardWidget {
        background-color: #1E1E1E;
        border: 1px solid #333333;
        border-radius: 8px;
        padding: 12px;
    }
    #cardTitle {
        font-size: 14px;
        font-weight: bold;
        color: #00E676;
        margin-bottom: 6px;
    }
    #metricValue {
        font-size: 20px;
        font-weight: bold;
        color: #FFFFFF;
    }
"""


class TelemetryApp(QMainWindow):

    def __init__(self):
        super().__init__()
        self.setWindowTitle("VATS - Telemetry & Control Dashboard")
        self.resize(1100, 680)

        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            self.sock.bind((UDP_IP, UDP_PORT_RX))
            self.sock.setblocking(False)
        except Exception as e:
            print(f"Ostrzeżenie gniazda UDP: {e}")

        self.manual_speed = 0
        self.manual_angle = 90
        self.pressed_keys = set()
        self.sim_tick = 0
        self.last_rx_timestamp = 0

        self.max_samples = 200
        self.time_data = list(range(self.max_samples))
        self.yaw_ref_data = [0.0] * self.max_samples
        self.yaw_meas_data = [0.0] * self.max_samples

        self.key_labels = {}

        self.init_ui()

        self.rx_timer = QTimer()
        self.rx_timer.timeout.connect(self.read_udp_data)
        self.rx_timer.start(10)  # 100 Hz

        self.tx_timer = QTimer()
        self.tx_timer.timeout.connect(self.send_manual_commands)
        self.tx_timer.start(50)  # 20 Hz

        self.conn_watchdog_timer = QTimer()
        self.conn_watchdog_timer.timeout.connect(self.update_connection_status_ui)
        self.conn_watchdog_timer.start(200)  # Co 200 ms

    def init_ui(self):
        self.setStyleSheet(DARK_STYLESHEET)

        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        main_layout = QVBoxLayout(main_widget)
        main_layout.setContentsMargins(15, 15, 15, 15)
        main_layout.setSpacing(12)

        top_bar = QHBoxLayout()

        top_bar.addWidget(QLabel("Tryb Pracy:"))
        self.mode_select = QComboBox()
        self.mode_select.addItems(
            ["1. Manual Control (WSAD)", "2. Autonomous ESP Telemetry"]
        )
        self.mode_select.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        self.mode_select.currentIndexChanged.connect(self.on_mode_changed)
        top_bar.addWidget(self.mode_select)

        top_bar.addSpacing(20)

        self.sim_checkbox = QCheckBox("Tryb Symulacji (Test UI)")
        self.sim_checkbox.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        top_bar.addWidget(self.sim_checkbox)

        top_bar.addStretch()

        self.status_label = QLabel("🔴 BRAK POŁĄCZENIA Z POJAZDEM")
        self.status_label.setObjectName("statusLabel")
        top_bar.addWidget(self.status_label)

        main_layout.addLayout(top_bar, stretch=0)

        self.stacked_widget = QStackedWidget()

        manual_page = QWidget()
        manual_layout = QHBoxLayout(manual_page)
        manual_layout.setContentsMargins(0, 0, 0, 0)
        manual_layout.setSpacing(15)

        keys_card = QWidget()
        keys_card.setObjectName("cardWidget")
        keys_card_layout = QVBoxLayout(keys_card)

        keys_title = QLabel("AKTYWNE STEROWANIE (WSAD)")
        keys_title.setObjectName("cardTitle")
        keys_title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        keys_card_layout.addWidget(keys_title)

        grid_layout = QGridLayout()
        grid_layout.setSpacing(10)

        for key_char in ["W", "A", "S", "D"]:
            lbl = QLabel(key_char)
            lbl.setFixedSize(70, 70)
            lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
            lbl.setStyleSheet(STYLE_KEY_RELEASED)
            self.key_labels[key_char] = lbl

        grid_layout.addWidget(self.key_labels["W"], 0, 1, Qt.AlignmentFlag.AlignCenter)
        grid_layout.addWidget(self.key_labels["A"], 1, 0, Qt.AlignmentFlag.AlignCenter)
        grid_layout.addWidget(self.key_labels["S"], 1, 1, Qt.AlignmentFlag.AlignCenter)
        grid_layout.addWidget(self.key_labels["D"], 1, 2, Qt.AlignmentFlag.AlignCenter)

        keys_card_layout.addStretch()
        keys_card_layout.addLayout(grid_layout)
        keys_card_layout.addStretch()

        manual_layout.addWidget(keys_card, stretch=2)

        info_card = QWidget()
        info_card.setObjectName("cardWidget")
        info_layout = QVBoxLayout(info_card)

        info_title = QLabel("TELEMETRIA NA ŻYWO")
        info_title.setObjectName("cardTitle")
        info_layout.addWidget(info_title)

        info_layout.addWidget(QLabel("Zadana Moc Silników (Moc PWM):"))
        self.val_speed = QLabel("0 %")
        self.val_speed.setObjectName("metricValue")
        info_layout.addWidget(self.val_speed)

        info_layout.addWidget(self.create_h_line())

        info_layout.addWidget(QLabel("Kąt Skrętu Osi Przedniej (Serwo):"))
        self.val_angle = QLabel("90° (Prosto)")
        self.val_angle.setObjectName("metricValue")
        info_layout.addWidget(self.val_angle)

        info_layout.addWidget(self.create_h_line())

        info_layout.addWidget(QLabel("Prędkość Obrotowa Kół (Enkodery N20):"))
        self.val_encoders = QLabel("L: 0 RPM  |  P: 0 RPM")
        self.val_encoders.setObjectName("metricValue")
        info_layout.addWidget(self.val_encoders)

        info_layout.addWidget(self.create_h_line())

        info_layout.addWidget(QLabel("Praca Stabilizatora Trajektorii (ESP):"))
        self.val_esp_offset = QLabel("Korekta Torque: 0 PWM")
        self.val_esp_offset.setObjectName("metricValue")
        info_layout.addWidget(self.val_esp_offset)

        info_layout.addStretch()
        manual_layout.addWidget(info_card, stretch=3)

        self.stacked_widget.addWidget(manual_page)

        auto_page = QWidget()
        auto_layout = QVBoxLayout(auto_page)
        auto_layout.setContentsMargins(0, 0, 0, 0)

        pg.setConfigOption("background", "#181818")
        pg.setConfigOption("foreground", "#CCCCCC")
        pg.setConfigOptions(antialias=True)

        self.plot_widget = pg.PlotWidget(
            title="<span style='color: #FFFFFF; font-size: 14px; font-weight: bold;'>Stabilizacja Trajektorii: Zadana vs Zmierzona Prędkość Kątowa (Yaw Rate)</span>"
        )
        self.plot_widget.setLabel("left", "Prędkość Kątowa ω_z", units="deg/s", color="#CCCCCC")
        self.plot_widget.setLabel("bottom", "Próbki", units="n", color="#CCCCCC")
        self.plot_widget.showGrid(x=True, y=True, alpha=0.25)
        self.plot_widget.setYRange(-60, 60)
        self.plot_widget.addLegend(offset=(10, 10))

        self.curve_ref = self.plot_widget.plot(
            self.time_data,
            self.yaw_ref_data,
            pen=pg.mkPen(color="#FF5252", width=2, style=Qt.PenStyle.DashLine),
            name="Zadana Kinematycznie (yaw_ref)",
        )
        self.curve_meas = self.plot_widget.plot(
            self.time_data,
            self.yaw_meas_data,
            pen=pg.mkPen(color="#00E676", width=2),
            name="Zmierzona IMU MPU-6500 (yaw_meas)",
        )

        auto_layout.addWidget(self.plot_widget)
        self.stacked_widget.addWidget(auto_page)

        main_layout.addWidget(self.stacked_widget, stretch=1)
        self.setFocus()

    def create_h_line(self):
        line = QFrame()
        line.setFrameShape(QFrame.Shape.HLine)
        line.setFrameShadow(QFrame.Shadow.Sunken)
        line.setStyleSheet("background-color: #2C2C2C; max-height: 1px;")
        return line

    def update_connection_status_ui(self):
        if self.sim_checkbox.isChecked():
            self.status_label.setText("🟡 TRYB SYMULACJI (TEST UI)")
            self.status_label.setStyleSheet(
                "color: #FFD600; background-color: #1E1E1E; border: 1px solid #FFD600;"
            )
            return

        if time.time() - self.last_rx_timestamp < 1.5:
            self.status_label.setText(f"🟢 POŁĄCZONO Z ESP32 ({VEHICLE_IP})")
            self.status_label.setStyleSheet(
                "color: #00E676; background-color: #1E1E1E; border: 1px solid #00E676;"
            )
        else:
            self.status_label.setText("🔴 BRAK POŁĄCZENIA Z POJAZDEM")
            self.status_label.setStyleSheet(
                "color: #FF1744; background-color: #1E1E1E; border: 1px solid #FF1744;"
            )

    def on_mode_changed(self, index):
        self.stacked_widget.setCurrentIndex(index)
        cmd = {"mode": index, "speed": 0, "angle": 90, "estop": 0}
        self.send_udp_json(cmd)
        self.setFocus()

    def keyPressEvent(self, event):
        if not event.isAutoRepeat():
            self.pressed_keys.add(event.key())
            self.update_manual_state()

    def keyReleaseEvent(self, event):
        if not event.isAutoRepeat() and event.key() in self.pressed_keys:
            self.pressed_keys.remove(event.key())
            self.update_manual_state()

    def update_manual_state(self):
        key_mapping = {
            Qt.Key.Key_W: "W",
            Qt.Key.Key_A: "A",
            Qt.Key.Key_S: "S",
            Qt.Key.Key_D: "D",
        }

        for qt_key, char in key_mapping.items():
            if qt_key in self.pressed_keys:
                self.key_labels[char].setStyleSheet(STYLE_KEY_PRESSED)
            else:
                self.key_labels[char].setStyleSheet(STYLE_KEY_RELEASED)

        if self.mode_select.currentIndex() != 0:
            return

        if Qt.Key.Key_W in self.pressed_keys:
            self.manual_speed = 70
        elif Qt.Key.Key_S in self.pressed_keys:
            self.manual_speed = -70
        else:
            self.manual_speed = 0

        if Qt.Key.Key_A in self.pressed_keys:
            self.manual_angle = 65
            angle_str = "65° (W Lewo)"
        elif Qt.Key.Key_D in self.pressed_keys:
            self.manual_angle = 115
            angle_str = "115° (W Prawo)"
        else:
            self.manual_angle = 90
            angle_str = "90° (Prosto)"

        self.val_speed.setText(f"{self.manual_speed} %")
        self.val_angle.setText(angle_str)

    def send_manual_commands(self):
        if self.mode_select.currentIndex() == 0:
            cmd = {
                "mode": 0,
                "speed": self.manual_speed,
                "angle": self.manual_angle,
                "estop": 0,
            }
            self.send_udp_json(cmd)

    def send_udp_json(self, data_dict):
        try:
            msg = json.dumps(data_dict).encode("utf-8")
            self.sock.sendto(msg, (VEHICLE_IP, VEHICLE_PORT_TX))
        except Exception:
            pass

    def read_udp_data(self):
        if self.sim_checkbox.isChecked():
            self.sim_tick += 1
            yaw_ref = 35.0 * math.sin(self.sim_tick * 0.05)
            yaw_meas = (
                35.0 * math.sin((self.sim_tick - 3) * 0.05)
                + random.uniform(-1.5, 1.5)
            )

            rpm_l = int(200 + yaw_meas * 1.2)
            rpm_r = int(200 - yaw_meas * 1.2)
            t_offset = int((yaw_ref - yaw_meas) * 2.5)

            self.update_telemetry_ui(
                yaw_ref, yaw_meas, rpm_l, rpm_r, t_offset
            )
            return

        try:
            while True:
                data, addr = self.sock.recvfrom(1024)
                payload = json.loads(data.decode("utf-8"))

                self.last_rx_timestamp = time.time()  # Aktualizacja czasu odbioru

                yaw_ref = payload.get("yaw_ref", 0.0)
                yaw_meas = payload.get("yaw_meas", 0.0)
                rpm_l = payload.get("rpm_l", 0)
                rpm_r = payload.get("rpm_r", 0)
                t_offset = payload.get("t_offset", 0)

                self.update_telemetry_ui(
                    yaw_ref, yaw_meas, rpm_l, rpm_r, t_offset
                )

        except (BlockingIOError, socket.error):
            pass
        except Exception as e:
            print("Błąd odebranej ramki:", e)

    def update_telemetry_ui(self, yaw_ref, yaw_meas, rpm_l, rpm_r, t_offset):
        self.yaw_ref_data.pop(0)
        self.yaw_ref_data.append(yaw_ref)

        self.yaw_meas_data.pop(0)
        self.yaw_meas_data.append(yaw_meas)

        self.curve_ref.setData(self.time_data, self.yaw_ref_data)
        self.curve_meas.setData(self.time_data, self.yaw_meas_data)

        self.val_encoders.setText(f"L: {rpm_l} RPM  |  P: {rpm_r} RPM")
        self.val_esp_offset.setText(f"Korekta Torque: {t_offset} PWM")

    def closeEvent(self, event):
        cmd = {"mode": 0, "speed": 0, "angle": 90, "estop": 1}
        self.send_udp_json(cmd)
        time.sleep(0.05)
        self.sock.close()
        event.accept()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = TelemetryApp()
    window.show()
    sys.exit(app.exec())