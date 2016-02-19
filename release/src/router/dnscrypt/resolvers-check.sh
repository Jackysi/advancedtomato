#! /bin/sh

RESOLVERS_LIST=dnscrypt-resolvers.csv
ONLINE_RESOLVERS_LIST=dnscrypt-online-resolvers.csv
DNSCRYPT_PROXY=dnscrypt-proxy
MARGIN=1440

tmpfile=$(mktemp .${ONLINE_RESOLVERS_LIST}.XXXXXXXXXXXX) || exit 1
trap "rm -f ${tmpfile}" EXIT

exec < "$RESOLVERS_LIST"
exec > "$tmpfile"

read header
echo "$header"

while read line; do
  resolver_name=$(echo "$line" | cut -d, -f1)
  eval "${DNSCRYPT_PROXY} -R ${resolver_name} -t ${MARGIN} -m 1"
  if [ $? -eq 0 ]; then
    echo "$line"
    echo "+ ${resolver_name} - OK" >&2
  else
    echo "- ${resolver_name} - Failed" >&2
  fi
done

mv -f "$tmpfile" "$ONLINE_RESOLVERS_LIST"
