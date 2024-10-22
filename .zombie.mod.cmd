savedcmd_/home/mkuo/CSE330/project4/zombie.mod := printf '%s\n'   zombie.o | awk '!x[$$0]++ { print("/home/mkuo/CSE330/project4/"$$0) }' > /home/mkuo/CSE330/project4/zombie.mod
