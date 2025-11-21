---

# 🛠️ OpenCBP_FO4  
### Game Version: `1.11.159`  
### F4SE Version: `0.7.5`  

## 🔄 Changes

### F4SE
-update f4se code to 0.7.5
--update f4se.vcxproj
--update f4se_common.vcxproj
---Add include path "$(SolutionDir)\..\.." instead of modifying utilities.h
### CBPSSE
-update cbpsse.vcxproj for 1.11.159 / 0x010B09F0
-update main.cpp for runtime version
-Address Library reveals New magic number is 0x1B18B40 

---

### Game Version: `1.11.137`  
### F4SE Version: `0.7.4`  

## 🔄 Changes

### `\common`
- `common\common_vc11.vcxproj`  
  - Toolset targeting changes (`v143`)

### `\f4se`
- Replaced entire `f4se` folder with `0.7.4` release  
- `f4se.vcxproj`  
  - `ProjectReference Include` changed from `..\..\common\common\common_vc11.vcxproj` to `..\..\common\common_vc14.vcxproj`  
  - Toolset targeting to `v143`  
  - `TargetName` changed to `$(ProjectName)_1_11_137`  
  - Added `IncludePath "$(SolutionDir)f4se"` (2 places)  
  - Removed `PostBuildEvent` blocks that copied the DLL to the Fallout 4 folder
  - In `ClCompile` block matching `Release|x64`, added:  
    ```xml
    <LanguageStandard>stdcpp17</LanguageStandard>
    ```

### `\f4se_common\f4se_common.vcxproj`
- Changed `Include` to `..\..\common\common_vc14.vcxproj`  
- Toolset `v143`

### `utilities.h`
- Changed include to `"f4se/f4se_common/Relocation.h"`

---

### `\CBPSSE`

#### `ActorUtils.cpp`
- Replaced `actorUtils::EquippedArmor` routine due to `Actor->equipData` change in `0.7.4`  
  - Replacement written by AI

#### `HookD3D.cpp`
- Replaced hook offset with new address (from AddressLibrary update)  
  - Next time, look for index `2287625` in Address Library  
  - Use `addresslibdecoder` from Common-FO4 tools to decode bin files and locate ID `2287625`
  - TODO: Investigate Address Library integration

#### `main.cpp`
- Updated `F4SEPluginVersionData`  
- Set `RUNTIME_VERSION_1_11_137`

#### `cbpsse.vcxproj`
- Set language standard to `stdcpp17`  
- Updated preprocessor definition:  
  - `RUNTIME_VERSION` to `0x010B0890` (was `0x010A3D80`)  
- Updated F4SE dependency name to `F4SE_1_11_137.LIB`  
- Removed post-build step to copy DLL to Fallout 4 folder  
- Added `"..\f4se\f4se\GameData.cpp"` to `ItemGroup` to resolve `DataHandler` link errors

#### `simobj.h`
- Defined `_SILENCE_AMP_DEPRECATION_WARNINGS` to appease Visual Studio

#### `common.vcxproj`
- Added include path `".."` to target `f4se` folder  
- Adjusted for compiling `f4se` from its parent folder

#### Misc
- Other compiler retargeting changes for toolset `v143`

---

