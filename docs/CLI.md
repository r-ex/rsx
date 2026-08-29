# Command-Line Interface

RSX features a range of command-line arguments that can be passed to the application to perform actions automatically, without creating a GUI.

The most important of these arguments is `-nogui`, which prevents window creation and initialization of DirectX/Dear ImGui, as most of the others will not work without it.

## Argument List

### Flags

These arguments do not take a value. Including them in the arguments provided to RSX will set their value to true.

| Flag                | Description                                                                                                         |
| ------------------- | ------------------------------------------------------------------------------------------------------------------- |
| -nogui              | Prevents creation of a graphical user interface                                                                     |
| -export             | Enables exporting of assets from the specified asset container files                                                |
| -matltextures       | Enables exporting of textures that are referenced by material assets in RPak files, when the materials are exported |
| -exportfullpaths    | Enables exporting of assets to the full path contained in their file name, instead of the usual asset type folders  |
| -exportdependencies | Enables exporting of all assets that are dependencies of other exported assets<sup>1</sup>                          |
| -exportrigsequences | Exports animation sequences that are associated with exported animation rigs<sup>1</sup>                            |
| -nocachedb          | Disables loading and usage of RSX's [cache database](./CacheDB.md)                                                  |
| -truncatedmodelmats | Enables shortening of material paths that are used by models, when the models are exported                          |
| -useqci             | When exporting models as [SMD](./assets/rpak/Model.md#smd), splits the QC file into multiple QCI files              |

<sup>1</sup> This is only used when an export filter is specified, as by default all assets are exported.

### Other

| Argument        | Values                                         | Description                                                                                                      |
| --------------- | ---------------------------------------------- | ---------------------------------------------------------------------------------------------------------------- |
| --nmlrecalc     | none, directx, opengl                          | Sets the type of normal-map recalculation that should be used when exporting materials                           |
| --texturenames  | guid, stored, text, semantic                   | Sets the format of the names of exported/previewed texture assets. See below<sup>2</sup>                         |
| --qcmajor       | (16-bit integer)                               | When exporting models as [SMD](./assets/rpak/Model.md#smd), sets the major version of the QC file                |
| --qcminor       | (16-bit integer)                               | When exporting models as [SMD](./assets/rpak/Model.md#smd), sets the minor version of the QC file                |
| --exportdir     | (string, directory path)                       | Sets the directory that all assets are exported to                                                               |
| --exporttypes   | (string, [asset type](./AssetTypes.md))        | If the `-export` flag is specified, Sets a list of types that should be exported                                 |
| --loadwhitelist | (string, [asset type](./AssetTypes.md))        | nogui only: parse/load only these types (comma-separated). If omitted, all types stay enabled                    |
| --exportthreads | (integer, min: `1`, default: `1`)              | Sets the number of threads to use for exporting. Clamped to the number of threads supported by the processor     |
| --parsethreads  | (integer, min: `1`, default: `0.5*maxThreads`) | Sets the number of threads to use for loading files. Clamped to the number of threads supported by the processor |
| --list | (string, file path) | Sets the file path that the loaded asset list will be saved as |
| --listformat | csv, txt | Sets the format that the asset list will be exported as |


### List
If `--list` is specified on the command line with a file path (e.g., `rsx --list "C:/asset_list.txt" common.rpak common_early.rpak ui.rpak`), the specified files (in this case: `common, common_early, ui`) will have their contents saved to the provided path as either a TXT or CSV file, depending on the value of the `--listformat` argument.

#### TXT
If the TXT option is chosen, the list will be a simple newline-separated text file with the asset names of all loaded assets as they would appear in the RSX GUI asset list.

#### CSV
If the CSV option is chosen, the list will be a Comma-Separated Values (CSV) file, with the following header:

`type,guid,file_name,asset_name`

- type: the type of asset, indicated by a string of up to 4 characters (e.g., `txtr`, `matl`, `mdl_`, `ui`, `shdr`)
- guid: a unique identifier of the asset
- file_name: the name of the container file that holds the asset (e.g., `common.rpak`, `ui.rpak`)
- asset_name: the name of the asset, as displayed in RSX's GUI asset list

