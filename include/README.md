# Include Layout

The upstream `lightning-lm` source uses `src/` as its public include root, for example `#include "core/localization/localization.h"` and `#include "common/point_def.h"`.

Stage one preserves that include layout to avoid behavior-changing header moves. This `include/` directory is intentionally kept as a package-level marker for downstream packaging conventions; the build target exports `${PROJECT_SOURCE_DIR}/src` and `${PROJECT_SOURCE_DIR}/thirdparty` as include roots.

