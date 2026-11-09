#ifndef SUMMARY_H
#define SUMMARY_H

#include <stdint.h>

// Calculate start (Monday 00:00:00) and end (Sunday 23:59:59.999) timestamps in ms for a given year & week
int summary_get_week_range(int year, int week_num, int64_t *out_start_ms, int64_t *out_end_ms);

// Get current year and ISO week number
void summary_get_current_week(int *out_year, int *out_week);

// Generate personal weekly summary (across all projects, includes user's private & public items)
int summary_generate_personal(const char *user_id, int64_t start_ms, int64_t end_ms, char **out_markdown);

// Generate project-wide weekly summary (groups by member, strictly excludes private items)
int summary_generate_project(const char *project_id, int64_t start_ms, int64_t end_ms, char **out_markdown);

// Free allocated markdown string
void summary_free(char *markdown);

#endif // SUMMARY_H
