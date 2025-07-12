SCRIPT_PATH=$(realpath $0)
current_dir=$(dirname $SCRIPT_PATH)

while [ "${current_dir}" != "/" ]; do
    if [ -e "${current_dir}/source" ] && [ -e "${current_dir}/misc" ]; then
        break
    fi

    current_dir=$(dirname ${current_dir})
done

if [ "${current_dir}" == "/" ]; then
    exit 1
fi

echo "export WINEPREFIX=${current_dir}/build/.wine"
