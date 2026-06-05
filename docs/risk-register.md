# Risk Register

| ID | Risk | Impact | Mitigation | Validation status |
|---|---|---|---|---|
| R1 | Current workspace has no ROS2 or colcon | Cannot prove build or runtime locally | Provide exact manual validation steps | Pending manual ROS2 environment validation |
| R2 | No complete bag/map data in workspace | Cannot replay localization locally | Require same config, map, and bag as upstream comparison | Pending manual ROS2 environment validation |
| R3 | Pangolin/OpenGL availability varies by target machine | UI build or runtime can fail | Keep dependency explicit and document environment setup | Pending manual ROS2 environment validation |
| R4 | `LocSystem::~LocSystem()` inherits non-null assumption for `loc_` | Failed init path may be fragile | Preserve behavior in stage one; record for industrial hardening | Static risk recorded |
| R5 | Online node subscribes to standard point cloud and Livox custom topic at the same time | Duplicate point cloud input if both topics carry same sensor stream | Deployment must configure active data source clearly | Pending manual ROS2 environment validation |
| R6 | `LocCmd.srv` is preserved but not registered in `LocSystem` | External relocalization service is not active | Document as inherited stage-one boundary | Static risk recorded |
| R7 | Debug PCD dump can add online disk IO when enabled | Realtime latency and file overwrite risk if operators enable high-frequency dumps | Default disabled through `debug_dump`; enable only for targeted troubleshooting | Configured off by default |
| R8 | PGO YAML fields may not fully drive current PGO implementation | Config expectations can diverge from runtime | Preserve original behavior and document parameter caveat | Static risk recorded |
| R9 | Copied service package name changes from upstream `lightning` to `lightning_localization` | Downstream code including generated service headers may need package-name update | Localization runtime does not include these headers; document interface boundary | Pending manual integration validation |
| R10 | Filesystem directory and ROS package name are both `lightning_localization` | Previous stage-one notes used an obsolete directory name | Keep docs aligned with the actual package directory | Updated in topic-output enhancement |
| R11 | Master prompt referenced `plans/stage-one-extraction-plan.md`, which was missing from the initial workspace | Execution package evidence could be incomplete | Recreated the plan from the master prompt and current package evidence | Static risk recorded |
