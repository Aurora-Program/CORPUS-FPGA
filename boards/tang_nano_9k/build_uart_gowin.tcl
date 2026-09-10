set project_file [file normalize [file join [pwd] corpus_uart.gprj]]
open_project $project_file
run all
exit
