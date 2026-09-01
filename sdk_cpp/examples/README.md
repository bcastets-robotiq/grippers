# Examples

**Start here: `move_gripper`** — the pattern every application should
use: a `Gripper` owns the bus; control code calls instant
setters and getters.

- `quick_start` — the minimal connect/activate/move sequence from the
  [Quick start](../../docs/2-Quick%20start.md) guide, with no error
  handling. Read alongside that guide.
- `move_gripper` — activate, close, and open a gripper through
  `Gripper`'s typed accessors, with error handling and logging. See the
  [walkthrough](../../docs/4-Robust%20example%20walkthrough.md).

Both are built by `GRIPPERS_BUILD_EXAMPLES` (see
[Environment setup](../../docs/1-Environment%20setup.md)).
