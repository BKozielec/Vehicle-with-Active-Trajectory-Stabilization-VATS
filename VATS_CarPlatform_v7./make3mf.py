import sys, cadquery as cq, vats_chassis as v, zipfile
OUT = sys.argv[1] if len(sys.argv) > 1 else "VATS_plate.3mf"
P = v.build()
def place(s, x, y, rot=0):
    s = s.rotate((0,0,0),(0,0,1),rot) if rot else s
    bb = s.val().BoundingBox()
    return s.translate((x-bb.xmin, y-bb.ymin, -bb.zmin))
items = [place(P["chassis"], 7, 20),
         place(P["motor_cap"], 7, 134), place(P["motor_cap"], 40, 134),
         place(P["knuckle_L"], 75, 134), place(P["knuckle_R"], 118, 134),
         place(P["tie_rod"], 160, 134, 90), place(P["drag_link"], 160, 148, 90)]
allc = cq.Workplane("XY").add(cq.Compound.makeCompound([i.val() for i in items]))
bb = allc.val().BoundingBox(); print("plate bbox", round(bb.xmin),round(bb.xmax),round(bb.ymin),round(bb.ymax),round(bb.zmax))
for a in range(len(items)):
    for b in range(a+1, len(items)):
        A=items[a].val().BoundingBox(); B=items[b].val().BoundingBox()
        if A.xmin<B.xmax and B.xmin<A.xmax and A.ymin<B.ymax and B.ymin<A.ymax: print("OVERLAP", a, b)
cq.exporters.export(allc, OUT, tolerance=0.02, angularTolerance=0.1)
m = zipfile.ZipFile(OUT).read("3D/3dmodel.model").decode()
print("objects:", m.count("<object "))
