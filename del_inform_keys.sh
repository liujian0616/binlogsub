#!/bin/sh

rediscli=/usr/local/bin/redis-cli
opt="-h 192.168.1.118 -p 20000"
#opt="-h 10.0.200.168 -p 6379"

del_staff() {
    echo "del_staff"
    ${rediscli} ${opt} keys group_staff_set* |xargs ${rediscli} ${opt} del
    ${rediscli} ${opt} keys user_groupid* |xargs ${rediscli} ${opt} del
}

del_friended() {
    echo "del_friended"
    ${rediscli} $opt keys friended_set* |xargs ${rediscli} ${opt} del
}

del_cscrop() {
    echo "del_cscrop"
    ${rediscli} ${opt} keys corp_cs_set* |xargs ${rediscli} ${opt} del
    ${rediscli} ${opt} keys corp_csleader_set* |xargs ${rediscli} ${opt} del
    ${rediscli} ${opt} keys cs_corpid* |xargs ${rediscli} ${opt} del
    ${rediscli} ${opt} keys cs_role* |xargs ${rediscli} ${opt} del
    ${rediscli} ${opt} keys cs_scheme_set* |xargs ${rediscli} ${opt} del
}

del_binlogpos() {
    echo "del_cscrop"
    ${rediscli} ${opt} del informhubsvr_binlog_pos_hash
}

del_all() {
    echo "del_all"
    del_binlogpos
    del_staff
    del_friended
    del_cscrop
}

if [ $# -eq 0 ]; then
    del_all
fi



while [ $# -ne 0 ]; do
    if [ $1 = "-s" ]; then
        del_staff
    elif [ $1 = "-f" ]; then
        del_friended
    elif [ $1 = "-c" ]; then
        del_cscrop
    elif [ $1 = "-b" ]; then
        del_binlogpos
    fi
    shift
done

