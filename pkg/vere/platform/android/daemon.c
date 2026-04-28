/// @file
/// Android daemon implementation - daemon mode disabled
///
/// Android applications run within the Zygote process model and cannot
/// reliably use fork() for daemonization. Instead, vere runs directly
/// as a foreground service managed by Android's init system.

#include "noun.h"
#include "vere.h"

/* u3_daemon_init(): platform-specific daemon mode initialization.
**
** On Android, daemon mode is not supported. Applications run as services
** managed by Android's init (for system services) or as foreground
** services for user apps.
**
** This function is a no-op that leaves bot_f as NULL, meaning the
** boot completion callback mechanism is disabled.
*/
void
u3_daemon_init()
{
  // Android does not support traditional Unix daemon mode.
  // The process runs directly without forking.
  // u3_Host.bot_f remains NULL - no boot completion callback needed.
}
