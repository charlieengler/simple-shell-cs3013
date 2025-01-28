
echo Section 5 background processes
./lsh test_section5_a.sh &
./lsh test_section5_b.sh &
./lsh test_section5_c.sh &
wait
echo waited

