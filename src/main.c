#define _DEFAULT_SOURCE

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "logger.h"
#include "process_manager.h"
#include "signal_manager.h"

#define SHUTDOWN_TIMEOUT_SEC 5
#define MONITOR_INTERVAL_US 200000

static void	log_children_status(void)
{
	int				count = 0;
	ChildProcess	*children = get_child_processes(&count);
	char			msg[256];

	if (children == NULL || count <= 0)
	{
		log_event("INFO", "Status report: No child processes registered.");
		return ;
	}
	log_event("INFO", "=== Child Process Status Report ===");
	for (int i = 0; i < count; i++)
	{
		snprintf(msg, sizeof(msg),
			"Child '%s' | PID: %d | Running: %s | Restarts: %d/%d",
		children[i].name,
		(int) children[i].pid,
		children[i].is_running ? "YES" : "NO",
		children[i].restart_count,
		MAX_RESTARTS);
		log_event("INFO", msg);
	}
}

static void	graceful_shutdown(void)
{
	int				count = 0;
	ChildProcess	*children = get_child_processes(&count);
	char			msg[128];

	log_event("INFO", "Shutdown signal received. Starting graceful shutdown...");
	if (children == NULL || count <= 0)
	{
		log_event("INFO", "No child processes to terminate.");
		return ;
	}
	for (int i = 0; i < count; i++)
	{
		if (children[i].is_running == 1 && children[i].pid > 0)
		{
			if (kill(children[i].pid, SIGTERM) == 0)
			{
				snprintf(msg, sizeof(msg),
					"SIGTERM sent to child '%s' (PID: %d).",
				children[i].name, (int) children[i].pid);
				log_event("INFO", msg);
			}
			else if (errno == ESRCH)
			{
				int	status = 0;

				waitpid(children[i].pid, &status, WNOHANG);
				children[i].is_running = 0;
				snprintf(msg, sizeof(msg),
					"Child '%s' (PID: %d) already terminated before SIGTERM.",
				children[i].name, (int) children[i].pid);
				log_event("INFO", msg);
			}
			else
			{
				snprintf(msg, sizeof(msg),
					"Failed to send SIGTERM to child '%s' (PID: %d).",
				children[i].name, (int) children[i].pid);
				log_event("WARN", msg);
			}
		}
	}
	for (int elapsed = 0; elapsed < SHUTDOWN_TIMEOUT_SEC; elapsed++)
	{
		int	still_running = 0;

		for (int i = 0; i < count; i++)
		{
			int		status = 0;
			pid_t	result;

			if (children[i].is_running != 1 || children[i].pid <= 0)
			{
				continue ;
			}
			result = waitpid(children[i].pid, &status, WNOHANG);
			if (result == children[i].pid)
			{
				children[i].is_running = 0;
				snprintf(msg, sizeof(msg),
					"Child '%s' (PID: %d) exited gracefully.",
				children[i].name, (int) children[i].pid);
				log_event("INFO", msg);
			}
			else if (result == -1 && errno == ECHILD)
			{
				children[i].is_running = 0;
			}
			else
			{
				still_running++;
			}
		}
		if (still_running == 0)
		{
			break ;
		}
		sleep(1);
	}
	for (int i = 0; i < count; i++)
	{
		int		status = 0;
		pid_t	result;

		if (children[i].is_running != 1 || children[i].pid <= 0)
		{
			continue ;
		}
		result = waitpid(children[i].pid, &status, WNOHANG);
		if (result == children[i].pid)
		{
			children[i].is_running = 0;
			snprintf(msg, sizeof(msg),
				"Child '%s' (PID: %d) exited just before timeout.",
			children[i].name, (int) children[i].pid);
			log_event("INFO", msg);
			continue ;
		}
		if (result == -1 && errno == ECHILD)
		{
			children[i].is_running = 0;
			continue ;
		}
		snprintf(msg, sizeof(msg),
			"Timeout reached. Sending SIGKILL to child '%s' (PID: %d).",
		children[i].name, (int) children[i].pid);
		log_event("WARN", msg);
		if (kill(children[i].pid, SIGKILL) == 0 || errno == ESRCH)
		{
			while (waitpid(children[i].pid, &status, 0) == -1 && errno == EINTR)
			{
			}
			children[i].is_running = 0;
			snprintf(msg, sizeof(msg),
				"Child '%s' (PID: %d) terminated with SIGKILL and reaped.",
			children[i].name, (int) children[i].pid);
			log_event("INFO", msg);
		}
		else
		{
			snprintf(msg, sizeof(msg),
				"Failed to send SIGKILL to child '%s' (PID: %d).",
			children[i].name, (int) children[i].pid);
			log_event("ERROR", msg);
		}
	}
}

int	main(void)
{
	if (init_logger("watchdog.log") != 0)
	{
		fprintf(stderr, "Error: Could not initialize logger.\n");
		return (EXIT_FAILURE);
	}
	if (setup_signal_handlers() != 0)
	{
		log_event("ERROR", "Failed to set up signal handlers.");
		close_logger();
		return (EXIT_FAILURE);
	}
	if (start_child_processes() != 0)
	{
		log_event("ERROR", "Failed to start child processes.");
		close_logger();
		return (EXIT_FAILURE);
	}
	log_event("INFO", "Watchdog started. Monitoring child processes...");
	while (!g_shutdown_flag)
	{
		if (g_status_report_flag == 1)
		{
			g_status_report_flag = 0;
			log_children_status();
		}
		monitor_and_restart_children();
		usleep(MONITOR_INTERVAL_US);
	}
	graceful_shutdown();
	log_event("INFO", "Watchdog shutdown complete.");
	close_logger();
	return (EXIT_SUCCESS);
}
