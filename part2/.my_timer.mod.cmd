cmd_/home/project2/part2/my_timer.mod := printf '%s\n'   my_timer.o | awk '!x[$$0]++ { print("/home/project2/part2/"$$0) }' > /home/project2/part2/my_timer.mod
