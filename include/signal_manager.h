#ifndef SIGNAL_MANAGER_H
#define SIGNAL_MANAGER_H

#include <signal.h>

extern volatile sig_atomic_t g_shutdown_flag;
extern volatile sig_atomic_t g_status_report_flag;

int setup_signal_handlers(void);

#endif
