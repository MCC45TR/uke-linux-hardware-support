#!/usr/bin/env bash
# Execute the actual fallback block; no hardware, calibration or Python input.
set -Eeuo pipefail
source=${1:?Pass prepared libssc-sensor.c}
work=$(mktemp -d); trap 'rm -r -- "$work"' EXIT
{
cat <<'C'
#include <assert.h>
#include <stddef.h>
#include <string.h>
typedef float gfloat;
typedef int gboolean;
typedef int gint;
typedef size_t gsize;
#define TRUE 1
static unsigned warnings;
#define g_warning(...) (warnings++)
struct matrix { gfloat mount_matrix[3][3]; };
static void fallback(struct matrix *priv) {
C
awk '/\/\* Fallback to identity matrix in case matrix is all 0 \*\// {emit=1;found=1} emit && /break;/ {exit} emit {print} END {if(!found)exit 1}' "$source"
cat <<'C'
}
int main(void) {
 const struct matrix identity={{{1,0,0},{0,1,0},{0,0,1}}};
 struct matrix value={0};fallback(&value);
 assert(!memcmp(&value,&identity,sizeof(value)) && warnings==1);
 value=identity;fallback(&value);assert(warnings==1 && !memcmp(&value,&identity,sizeof(value)));
 // A 120-degree rotation about X has nonzero entries whose total sum is zero.
 const struct matrix rotated={{{1,0,0},{0,-.5f,-.8660254f},{0,.8660254f,-.5f}}};
 value=rotated;fallback(&value);assert(warnings==1 && !memcmp(&value,&rotated,sizeof(value)));
 const struct matrix fractional={{{.7071068f,-.7071068f,0},{.7071068f,.7071068f,0},{0,0,1}}};
 value=fractional;fallback(&value);assert(warnings==1 && !memcmp(&value,&fractional,sizeof(value)));
 for(unsigned i=0;i<9;i++) {
  value=(struct matrix){0};value.mount_matrix[i/3][i%3]=-.001f;
  struct matrix before=value;fallback(&value);
  assert(warnings==1 && !memcmp(&value,&before,sizeof(value)));
 }
 return 0;
}
C
} > "$work/check.c"
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror "$work/check.c" -o "$work/check"
"$work/check"
echo 'Actual libssc matrix fallback preserves nonzero rotations, including zero-sum and fractional values; only all-zero input uses identity and warns'
