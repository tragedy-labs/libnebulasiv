#include "hil_device.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "neb_core.h"

// ---------------------------------------------------------------------------
// Identification
// ---------------------------------------------------------------------------

// Model names as VERSIONA reports them, longest first: "UM960L" contains
// "UM960", so a shortest-first scan would misidentify the L variant.
static const struct {
  const char *name;
  neb_model_t model;
} known_models[] = {
    {"UM960L", NEB_MODEL_UM960L},
    {"UM982", NEB_MODEL_UM982},
    {"UM980", NEB_MODEL_UM980},
    {"UM960", NEB_MODEL_UM960},
};

// Copy the quoted field containing `inner` out of `text`, without its quotes.
// VERSIONA reports the firmware as a quoted field ("R4.10Build13504"), so this
// recovers the whole version string from a match on "Build".
static void copy_quoted_field(const char *text, const char *inner, char *out,
                              size_t out_size) {
  out[0] = '\0';

  const char *hit = strstr(text, inner);
  if (!hit)
    return;

  const char *start = hit;
  while (start > text && *(start - 1) != '"')
    start--;

  const char *end = hit;
  while (*end && *end != '"')
    end++;

  size_t len = (size_t)(end - start);
  if (len >= out_size)
    len = out_size - 1;
  memcpy(out, start, len);
  out[len] = '\0';
}

int hil_identify(const char *port, int baudrate, hil_device_t *device) {
  if (!port || !device)
    return -1;

  memset(device, 0, sizeof(*device));

  // The model is what we are trying to discover, so it cannot be an input
  // here. VERSIONA is sent through neb_send_command, which frames and sends
  // without consulting the capability bitfield -- so the model passed to
  // neb_open is irrelevant to this one exchange. The caller reopens with the
  // discovered model before running anything model-specific.
  neb_handle_t probe;
  if (neb_open(&probe, NEB_MODEL_UM980, port, baudrate) != NEB_OK)
    return -1;

  // VERSIONA's payload is far longer than any acknowledgement, so this buffer
  // is sized for the record rather than the ack.
  char response[2048];
  neb_status_t status =
      neb_send_command(&probe, "VERSIONA", response, sizeof(response));
  neb_close(&probe);

  if (status != NEB_OK)
    return -1;

  for (size_t i = 0; i < sizeof(known_models) / sizeof(known_models[0]); i++) {
    if (strstr(response, known_models[i].name)) {
      device->model = known_models[i].model;
      snprintf(device->model_name, sizeof(device->model_name), "%s",
               known_models[i].name);
      break;
    }
  }
  if (device->model_name[0] == '\0')
    return -1;

  copy_quoted_field(response, "Build", device->firmware,
                    sizeof(device->firmware));

  const char *build = strstr(response, "Build");
  if (build)
    device->build = (unsigned)strtoul(build + strlen("Build"), NULL, 10);

  return 0;
}

// ---------------------------------------------------------------------------
// Result recording
// ---------------------------------------------------------------------------

#define HIL_MAX_RESULTS 128

static struct {
  char test[64];
  char summary[128];
  char detail[160];
  hil_outcome_t outcome;
} g_results[HIL_MAX_RESULTS];

static size_t g_result_count;

static const char *outcome_name(hil_outcome_t outcome) {
  switch (outcome) {
  case HIL_PASS:
    return "pass";
  case HIL_FAIL:
    return "fail";
  case HIL_UNSUPPORTED:
    return "unsupported";
  case HIL_DISCREPANCY:
    return "discrepancy";
  case HIL_SKIP_BUILD:
    return "skip-build";
  case HIL_SKIP_LEVEL:
    return "skip-level";
  default:
    return "unknown";
  }
}

void hil_record(const char *test, const char *summary, hil_outcome_t outcome,
                const char *detail) {
  if (g_result_count >= HIL_MAX_RESULTS)
    return;

  snprintf(g_results[g_result_count].test, sizeof(g_results[0].test), "%s",
           test ? test : "");
  snprintf(g_results[g_result_count].summary, sizeof(g_results[0].summary),
           "%s", summary ? summary : "");
  snprintf(g_results[g_result_count].detail, sizeof(g_results[0].detail), "%s",
           detail ? detail : "");
  g_results[g_result_count].outcome = outcome;
  g_result_count++;
}

// Reduce free text to a filename-safe slug, so a board name supplied on the
// command line cannot decide where we write.
static void slugify(const char *text, char *out, size_t out_size) {
  size_t n = 0;
  int previous_was_dash = 1; // suppress a leading dash

  for (const char *p = text; *p && n + 1 < out_size; p++) {
    const int is_word = (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
                        (*p >= '0' && *p <= '9');
    if (is_word) {
      out[n++] = (char)(*p >= 'A' && *p <= 'Z' ? *p - 'A' + 'a' : *p);
      previous_was_dash = 0;
    } else if (!previous_was_dash) {
      out[n++] = '-';
      previous_was_dash = 1;
    }
  }

  while (n > 0 && out[n - 1] == '-') // no trailing dash
    n--;

  out[n] = '\0';
}

int hil_write_results(const hil_device_t *device, const char *dir,
                      const char *board, int baudrate, hil_level_t level,
                      const char *contributor) {
  if (!device || !dir)
    return -1;

  mkdir(dir, 0775); // may already exist; the fopen below is the real check

  char board_slug[64];
  slugify(board && board[0] ? board : "unspecified", board_slug,
          sizeof(board_slug));

  char model_slug[16];
  slugify(device->model_name, model_slug, sizeof(model_slug));

  char path[512];
  char temp_path[544];
  snprintf(path, sizeof(path), "%s/%s-build%u-%s.tsv", dir, model_slug,
           device->build, board_slug);
  snprintf(temp_path, sizeof(temp_path), "%s.tmp", path);

  FILE *file = fopen(temp_path, "w");
  if (!file)
    return -1;

  char today[16];
  const time_t now = time(NULL);
  struct tm calendar;
  if (localtime_r(&now, &calendar))
    strftime(today, sizeof(today), "%Y-%m-%d", &calendar);
  else
    snprintf(today, sizeof(today), "unknown");

  // Two record types, distinguished by the first field so the generator can
  // read this in one pass: M = run metadata, T = one test outcome.
  fprintf(file, "M\tmodel\t%s\n", device->model_name);
  fprintf(file, "M\tfirmware\t%s\n", device->firmware);
  fprintf(file, "M\tbuild\t%u\n", device->build);
  fprintf(file, "M\tboard\t%s\n", board && board[0] ? board : "(unspecified)");
  fprintf(file, "M\tinterface\tserial 8N1\n");
  fprintf(file, "M\tbaud\t%d\n", baudrate);
  fprintf(file, "M\tlevel\t%s\n", level == HIL_LEVEL_RAM ? "ram" : "read");
  fprintf(file, "M\tcontributor\t%s\n",
          contributor && contributor[0] ? contributor : "(unspecified)");
  fprintf(file, "M\tdate\t%s\n", today);

  for (size_t i = 0; i < g_result_count; i++)
    fprintf(file, "T\t%s\t%s\t%s\t%s\n", g_results[i].test,
            outcome_name(g_results[i].outcome), g_results[i].summary,
            g_results[i].detail);

  const int flushed = fclose(file);
  if (flushed != 0)
    return -1;

  if (rename(temp_path, path) != 0)
    return -1;

  fprintf(stderr, "\nRecorded %zu results to %s\n", g_result_count, path);
  return 0;
}
