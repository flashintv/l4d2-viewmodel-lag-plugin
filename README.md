# Left 4 Dead 1 and 2 client plugin that restores Half-Life 2 viewmodel lag and offset functionality

## Added commands:
  - Both games:
    - pl_wpn_sway_enabled (default: true)
    - pl_wpn_sway_scale (default: 1.5)
    - pl_wpn_sway_interp (default: 0.1)
  - Only in L4D1:
    - viewmodel_offset_x (default: 0.0)
    - viewmodel_offset_y (default: 0.0)
    - viewmodel_offset_z (default: 0.0)

## How to run:
Download the `viewmodel_lag_plugin.dll` from Releases and put it into the game's bin folder (example: "Steam/steamapps/common/left 4 dead/bin")
>| 1st Method: |
>|:- |
>| - To load the plugin, run in the console: |
>|`plugin_load viewmodel_lag_plugin`
>| - To unload the plugin run: |
>|`plugin_unload viewmodel_lag_plugin` |
>
>| 2nd Method: |
>|:- |
>| Download `viewmodel_lag_plugin.vdf` from Releases and put it within the mod's addons folder (example: "Steam/steamapps/common/left 4 dead/left4dead/addons"), the game will automatically load the plugin on startup. |

Running the plugin on a dedicated server, doesn't do anything - this plugin is made for the client only.

## How to build:
Clone the repository and open Everything_SDK-2013.sln with Visual Studio 2022 and compile.
The DLL binary is built using Visual Studio 2013 build tools, for best results and compatibility use the same version.

## Additional information:
This repository uses alliedmodders/hl2sdk branch l4d2 as a base and modifies a couple files from the original source code for compatibility with the serverplugin framework and VS2013 build tools as it's not compilable by default.
In addition the source code uses MinHook v1.3.3, a compiled binary is supplied.
