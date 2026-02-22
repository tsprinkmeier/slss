#!/bin/bash

# https://www.gnu.org/software/bash/manual/html_node/Shell-Parameter-Expansion.html

# exit on fail (set -e)
set -o errexit

# verbose (set -x)
set -o xtrace

## unset = error
#set -o nounset

# very verbose
PS4='+ $(date +%T) ($(basename ${BASH_SOURCE}):${LINENO}): ${FUNCNAME[0]:+${FUNCNAME[0]}(): }'

# log _everything_ to a file
exec > >( tee $( mktemp ) ) 2>&1

# directory containing this script
_ROOT_=$(dirname $(readlink --canonicalize ${BASH_SOURCE}))

# PIC
pushd ${_ROOT_}

# re-build
# make clobber
make

# smoke-test
./aont 1 2 || true
./gfm  1 2 || true
./slss 1 2 || true

DIR=$( mktemp --directory )

# Plainetxt
PLAINTEXT=$( date ; uptime ; free )
echo "${PLAINTEXT}" > ${DIR}/plaintext
md5sum < ${DIR}/plaintext > ${DIR}/md5sum
# AONT encrypt
# implicit and explicit STDIN to STDOUT
./aont    > "${DIR}/banana0.aont" < "${DIR}/plaintext"
./aont  - > "${DIR}/banana1.aont" < "${DIR}/plaintext"
# encrypt existing file
./aont                              "${DIR}/plaintext"
# encrypt STDIN to STUB.aont
./aont      "${DIR}/banana2"      < "${DIR}/plaintext"
# remove original plaintext
rm "${DIR}/plaintext"
# AONT decrypt
./aont "${DIR}/banana0.aont"
./aont "${DIR}/banana1.aont"
./aont "${DIR}/banana2.aont"
./aont "${DIR}/plaintext.aont"
# compare plaintext
md5sum --check ${DIR}/md5sum < ${DIR}/banana0
md5sum --check ${DIR}/md5sum < ${DIR}/banana1
md5sum --check ${DIR}/md5sum < ${DIR}/banana2
md5sum --check ${DIR}/md5sum < ${DIR}/plaintext

exit 0

echo banana | ./gfm ${DIR}/banana 3 2
pushd  ${DIR}/
tar --extract                 --file banana_01.gfm.tar
tar --extract --auto-compress --file slss.tar.xz
pushd ./slss*/
make
popd
./slss*/gfm banana
md5sum banana

