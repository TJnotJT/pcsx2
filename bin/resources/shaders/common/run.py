def preprocess(in_file, defs_file, vs_file, ps_file, out_file):
  with open(defs_file, "r") as file:
      defs_str = file.read()
  with open(vs_file, "r") as file:
    vs_str = file.read()
  with open(ps_file, "r") as file:
    ps_str = file.read()
  with open(in_file, "r") as file:
    in_str = file.read()
  in_str = in_str.replace('#include "tfx_defs.inc"', defs_str + "\n")
  in_str = in_str.replace('#include "tfx_vs.inc"', vs_str + "\n")
  in_str = in_str.replace('#include "tfx_ps.inc"', ps_str + "\n")
  with open(out_file, "w") as file:
    file.write(in_str)

if __name__ == "__main__":
  # preprocess("tfx_vk.glsl", "tfx_defs.inc", "tfx_vs.inc", "tfx_ps.inc", "tfx_vk_prep.glsl")
  preprocess("tfx_dx.fx", "tfx_defs.inc", "tfx_vs.inc", "tfx_ps.inc", "tfx_dx_prep.fx")