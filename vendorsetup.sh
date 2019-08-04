function setupjack() {
    # get amount of ram in kb
    ram=$(grep MemTotal /proc/meminfo | awk '{print $2}')
    # convert to gb
    ram=$(expr $ram / 1000000)
    # add gb sign
    g="G"
    ram="$ram$g"
    # export to jack
    export ANDROID_JACK_VM_ARGS="-Dfile.encoding=UTF-8 -XX:+TieredCompilation -Xmx$ram"
}

function mka() {
    # get number of cores
    cores=$(nproc)
    # 4 threads per core
    threads=$(expr $cores \* 4)
    m -j$threads "$@"
}
