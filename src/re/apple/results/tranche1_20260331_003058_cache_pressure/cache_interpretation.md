# Cache Pressure Interpretation

- pointer_chase_llc: min_ns_per_access=1.836260
  observed_knee: size_kib=192, stride_bytes=64, ns_per_access=2.450877
- reuse_stride_hotset: min_ns_per_access=4.408676
  observed_knee: size_kib=64, stride_bytes=128, ns_per_access=8.057117
- stream_l1_hot: min_ns_per_access=0.993571
  observed_knee: size_kib=16, stride_bytes=512, ns_per_access=1.169922
- stream_l2_boundary: min_ns_per_access=1.223882
  observed_knee: size_kib=192, stride_bytes=64, ns_per_access=1.542197
