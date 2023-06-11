if [[ $# -ne 2 ]] ; 
then
    echo "You should pass 2 args: work_time_1, work_time_2"
    exit 1
fi

./first_gardener "127.0.0.1" 8085 $1 &
./second_gardener "127.0.0.1" 8085 $2 &
