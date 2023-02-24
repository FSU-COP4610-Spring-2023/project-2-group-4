cmd_/home/project2/part2/Module.symvers := sed 's/ko$$/o/' /home/project2/part2/modules.order | scripts/mod/modpost -m -a  -o /home/project2/part2/Module.symvers -e -i Module.symvers   -T -
