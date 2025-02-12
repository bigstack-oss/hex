
./$TEST -ve commit bootstrap 2> test.out
grep "Committing modules (sys-done)" test.out
grep "Commit test1" test.out
grep "Commit test2" test.out
grep "Commit test3" test.out
grep "Commit test4" test.out
grep "Commit test5" test.out

./$TEST -ve commit bootstrap test1 2> test.out
cat test.out
grep "Committing modules (test1-test1)" test.out
grep "Commit test1" test.out
! grep "Commit test2" test.out
! grep "Commit test3" test.out
! grep "Commit test4" test.out
! grep "Commit test5" test.out

./$TEST -ve commit bootstrap test2-test4 2> test.out
grep "Committing modules (test2-test4)" test.out
! grep "Commit test1" test.out
grep "Commit test2" test.out
grep "Commit test3" test.out
grep "Commit test4" test.out
! grep "Commit test5" test.out

./$TEST -ve commit bootstrap test2-xxx 2> test.out
grep "Committing modules (test2-done)" test.out
! grep "Commit test1" test.out
grep "Commit test2" test.out
grep "Commit test3" test.out
grep "Commit test4" test.out
grep "Commit test5" test.out

./$TEST -ve commit bootstrap xxx-test4 2> test.out
grep "Committing modules (sys-test4)" test.out
grep "Commit test1" test.out
grep "Commit test2" test.out
grep "Commit test3" test.out
grep "Commit test4" test.out
! grep "Commit test5" test.out

./$TEST -ve commit bootstrap xxx-yyy 2> test.out
grep "Committing modules (sys-done)" test.out
grep "Commit test1" test.out
grep "Commit test2" test.out
grep "Commit test3" test.out
grep "Commit test4" test.out
grep "Commit test5" test.out

./$TEST -ve commit bootstrap xxx 2> test.out
grep "Committing modules (sys-done)" test.out
grep "Commit test1" test.out
grep "Commit test2" test.out
grep "Commit test3" test.out
grep "Commit test4" test.out
grep "Commit test5" test.out

cat </dev/null >test.txt

./$TEST -ve commit test.txt 2> test.out
grep "Committing modules (sys-done)" test.out
grep "Commit test1" test.out
grep "Commit test2" test.out
grep "Commit test3" test.out
grep "Commit test4" test.out
grep "Commit test5" test.out

./$TEST -ve commit test.txt test1 2> test.out
cat test.out
grep "Committing modules (test1-test1)" test.out
grep "Commit test1" test.out
! grep "Commit test2" test.out
! grep "Commit test3" test.out
! grep "Commit test4" test.out
! grep "Commit test5" test.out

./$TEST -ve commit test.txt test2-test4 2> test.out
grep "Committing modules (test2-test4)" test.out
! grep "Commit test1" test.out
grep "Commit test2" test.out
grep "Commit test3" test.out
grep "Commit test4" test.out
! grep "Commit test5" test.out

./$TEST -ve commit test.txt test2-xxx 2> test.out
grep "Committing modules (test2-done)" test.out
! grep "Commit test1" test.out
grep "Commit test2" test.out
grep "Commit test3" test.out
grep "Commit test4" test.out
grep "Commit test5" test.out

./$TEST -ve commit test.txt xxx-test4 2> test.out
grep "Committing modules (sys-test4)" test.out
grep "Commit test1" test.out
grep "Commit test2" test.out
grep "Commit test3" test.out
grep "Commit test4" test.out
! grep "Commit test5" test.out

./$TEST -ve commit test.txt xxx-yyy 2> test.out
grep "Committing modules (sys-done)" test.out
grep "Commit test1" test.out
grep "Commit test2" test.out
grep "Commit test3" test.out
grep "Commit test4" test.out
grep "Commit test5" test.out

./$TEST -ve commit test.txt xxx 2> test.out
grep "Committing modules (sys-done)" test.out
grep "Commit test1" test.out
grep "Commit test2" test.out
grep "Commit test3" test.out
grep "Commit test4" test.out
grep "Commit test5" test.out

touch /etc/hex_config.commit.all
./$TEST -ve commit test.txt test2-test4 2> test.out
grep "Committing modules (sys-done)" test.out
grep "Commit test1" test.out
grep "Commit test2" test.out
grep "Commit test3" test.out
grep "Commit test4" test.out
grep "Commit test5" test.out
rm -f /etc/hex_config.commit.all
