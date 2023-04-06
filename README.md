### listo
A small program to manage your task list

### Usage
If you've `man` installed, run `man ./listo.1` for the manual.\
Output of `./listo -h`
```
-a [i(0)..i(n)] -- Append item(s) in a already existing list file
-d              -- Print all existing list(s)
-e              -- Edit a list entry to make it current
-n [i(0)..i(n)] -- Create a new list with pre-adjusted item(s)
-o              -- Open the list file to a terminal-based editor
-p [t]          -- Print all items from the most recent file
-r              -- Purge all old list(s)
-l [lnum]       -- Delete a specific line from the current list
```

Note, each of these "i" is referred to an item. That means you can add more than one argument in one line which will be appended to a new line in the list file. By using quotes (""), you can append all of the passed arguments in one line.

### Others
If you want to send patches, you can also send them to my mailbox.

### Thanks
[jhx](https://github.com/jhx0) - Notified multiple issues, especially with error handling