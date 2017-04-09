VG = 127.0/128.0
CM = 1
R_EXT = 1000

V_REXT = 1.26 * VG
I_REF = V_REXT / R_EXT
I_OUT = I_REF * 15.0 * 3.0 ** (CM-1.0)

print I_OUT * 1000.0, "mA"
