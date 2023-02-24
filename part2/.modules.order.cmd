cmd_/home/project2/part2/modules.order := {   echo /home/project2/part2/my_timer.ko; :; } | awk '!x[$$0]++' - > /home/project2/part2/modules.order
