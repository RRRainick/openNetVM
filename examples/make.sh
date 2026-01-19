DIR=$(
  cd "$(dirname "$0")" || exit 1
  pwd
)

arr=(dmt*/)

for dir in "${arr[@]}"; do
  cd "$dir"
  make
  cd $DIR
done
