# MultiBMPRes Tool

MultiBMPRes Tool is a C-based utility for managing bitmap resources in a binary resource file. It allows users to add, list, and retrieve bitmap resources efficiently.

## Features

- Add new bitmap resources to a binary resource file.
- List all stored resources with dimensions and compression details.
- Retrieve and decompress bitmap data from the resource file.
- Command-line and interactive modes for ease of use.

## Usage
MultiBMPRes supports both an interactive CLI and a scripting mode. You can access interactive mode by running the program without any arguments.
### Interactive
Example:
```
MultiBMPRes Tool v1.00
(c) Noah Wooten, All Rights Reserved 2023-2025

1.) Add new resource
2.) List resources
3.) Exit

> 1
Resource file name: image0.bmp
Resource name: Image0
Added resource 'Image0', added 100 KiB. (50.00% compression)
```

### Scripting

#### Add Files
Syntax: `multibmpres.exe <resource_file> --add <bitmap_file> <resource_name>`

#### List Files
Syntax: `multibmpres.exe <resource_file> --list`

Expected output: 
```
MultiBMPRes Tool v1.00
(c) Noah Wooten, All Rights Reserved 2023-2025

1.) 'Image0' (640x480, 120 KiB, 300 KiB Uncompressed)
2.) 'Image1' (800x600, 150 KiB, 400 KiB Uncompressed)
```
