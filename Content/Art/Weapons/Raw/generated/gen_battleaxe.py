"""Generate a CC0 low-poly battle axe OBJ (crescent blade + haft — not a pickaxe)."""
import math
import os

out_dir = r"C:\Users\Lehel\shadowbanefps\Content\Art\Weapons\Raw\generated"
os.makedirs(out_dir, exist_ok=True)
path = os.path.join(out_dir, "SM_BattleAxe.obj")
mtl_path = os.path.join(out_dir, "SM_BattleAxe.mtl")

verts = []
faces = []

def add_v(x, y, z):
    verts.append((x, y, z))
    return len(verts)

def add_quad(a, b, c, d):
    faces.append((a, b, c, d))

def add_tri(a, b, c):
    faces.append((a, b, c))

haft_len = 1.05
haft_r = 0.028
sides = 8
ring_ids = []
for z in (0.0, haft_len):
    ring = []
    for i in range(sides):
        ang = (i / sides) * math.tau
        ring.append(add_v(math.cos(ang) * haft_r, math.sin(ang) * haft_r, z))
    ring_ids.append(ring)

for i in range(sides):
    i2 = (i + 1) % sides
    add_quad(ring_ids[0][i], ring_ids[0][i2], ring_ids[1][i2], ring_ids[1][i])

pommel_z = -0.04
pommel_r = 0.045
pommel_bot = add_v(0, 0, pommel_z - 0.03)
pommel_top = []
for i in range(sides):
    ang = (i / sides) * math.tau
    pommel_top.append(add_v(math.cos(ang) * pommel_r, math.sin(ang) * pommel_r, pommel_z))
for i in range(sides):
    i2 = (i + 1) % sides
    add_tri(pommel_bot, pommel_top[i2], pommel_top[i])
    add_quad(ring_ids[0][i], ring_ids[0][i2], pommel_top[i2], pommel_top[i])

socket_z = haft_len - 0.02
socket_r = 0.04
socket_h = 0.12
sock_lo, sock_hi = [], []
for i in range(sides):
    ang = (i / sides) * math.tau
    sock_lo.append(add_v(math.cos(ang) * socket_r, math.sin(ang) * socket_r, socket_z - socket_h * 0.5))
    sock_hi.append(add_v(math.cos(ang) * socket_r * 1.05, math.sin(ang) * socket_r * 1.05, socket_z + socket_h * 0.5))
for i in range(sides):
    i2 = (i + 1) % sides
    add_quad(sock_lo[i], sock_lo[i2], sock_hi[i2], sock_hi[i])

blade_center_z = socket_z
inner, outer = [], []
n_blade = 14
for i in range(n_blade):
    t = i / (n_blade - 1)
    z = blade_center_z + 0.22 - t * 0.55
    flare = 0.18 + 0.22 * math.sin(t * math.pi * 0.85) + 0.08 * t
    xi = 0.05
    crescent = flare + 0.06 * math.sin(t * math.pi)
    xo = xi + crescent
    inner.append(add_v(xi, 0.0, z))
    outer.append(add_v(xo, 0.0, z))

def extrude_chain(chain, y_off):
    out = []
    for idx in chain:
        x, y, z = verts[idx - 1]
        out.append(add_v(x, y_off, z))
    return out

inner_p = extrude_chain(inner, 0.02)
inner_n = extrude_chain(inner, -0.02)
outer_p = extrude_chain(outer, 0.01)
outer_n = extrude_chain(outer, -0.01)

for i in range(n_blade - 1):
    add_quad(inner_p[i], outer_p[i], outer_p[i + 1], inner_p[i + 1])
    add_quad(inner_n[i + 1], outer_n[i + 1], outer_n[i], inner_n[i])
    add_quad(outer_p[i], outer_n[i], outer_n[i + 1], outer_p[i + 1])
    add_quad(inner_n[i], inner_p[i], inner_p[i + 1], inner_n[i + 1])

add_quad(inner_p[0], inner_n[0], outer_n[0], outer_p[0])
add_quad(inner_n[-1], inner_p[-1], outer_p[-1], outer_n[-1])

# Short rear poll/hammer (battle axe), NOT a long pick spike
poll_len = 0.09
poll_z0 = blade_center_z - 0.04
poll_z1 = blade_center_z + 0.04
poll_y = 0.025
p = [
    add_v(-0.02, -poll_y, poll_z0),
    add_v(-0.02 - poll_len, -poll_y, poll_z0),
    add_v(-0.02 - poll_len, poll_y, poll_z0),
    add_v(-0.02, poll_y, poll_z0),
    add_v(-0.02, -poll_y, poll_z1),
    add_v(-0.02 - poll_len, -poll_y, poll_z1),
    add_v(-0.02 - poll_len, poll_y, poll_z1),
    add_v(-0.02, poll_y, poll_z1),
]
add_quad(p[0], p[1], p[2], p[3])
add_quad(p[4], p[7], p[6], p[5])
add_quad(p[0], p[3], p[7], p[4])
add_quad(p[1], p[5], p[6], p[2])
add_quad(p[3], p[2], p[6], p[7])
add_quad(p[0], p[4], p[5], p[1])

with open(mtl_path, "w", encoding="utf-8") as f:
    f.write("newmtl Wood\nKd 0.45 0.28 0.12\nKs 0.05 0.05 0.05\nNs 20\n\n")
    f.write("newmtl Steel\nKd 0.72 0.74 0.78\nKs 0.4 0.4 0.4\nNs 80\n")

scale = 100.0
with open(path, "w", encoding="utf-8") as f:
    f.write("# CC0 ShadowbaneFPS battle axe (crescent blade)\n")
    f.write("mtllib SM_BattleAxe.mtl\n")
    for x, y, z in verts:
        f.write(f"v {x * scale:.4f} {y * scale:.4f} {z * scale:.4f}\n")
    f.write("g BattleAxe\nusemtl Steel\n")
    for face in faces:
        if len(face) == 3:
            f.write(f"f {face[0]} {face[1]} {face[2]}\n")
        else:
            f.write(f"f {face[0]} {face[1]} {face[2]} {face[3]}\n")

print(f"Wrote {path} verts={len(verts)} faces={len(faces)}")
