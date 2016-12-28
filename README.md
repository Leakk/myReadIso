#MyReadISO

### NAME

**my_read_iso** - An ISO reader

### DESCRIPTION

**my_read_iso** is an ISO(9660) file reader, containing the following commands:
**help info ls cd tree get cat quit**(see COMMANDS).

### HOW TO USE

#### To Compile

>**42sh\$** cd myReadIso<br>
**42sh\$** make

#### To use

> **42sh\$** ./my_read_iso **[*your ISO file*]**

or

> **42sh\$** ls cmdfile<br>
> ls<br>
> quit<br>
> **42sh\$** ./my-read-iso **[*your ISO file*]** < cmdfile

### COMMANDS

**help** display command help<br>
**info** display volume info<br>
**ls** display directory content<br>
**cd** change current directory<br>
- **cd** go back to root dir
- **cd ..**  go to the parent dir
- **cd -**  go to the prevese dir

**tree**  display the tree of the current directory<br>
**get** copy file to local directory<br>
**cat** display file content<br>
**quit** program exit
