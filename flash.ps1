# Flash out/pwr-probe-test/default.hex to PIC24EP256MC206 via PICkit 4
# Usage: .\flash.ps1
# Requires: PICkit 4 connected, target powered or use -W flag for PICkit-supplied power

$ipecmd  = "C:\Program Files\Microchip\MPLABX\v6.20\mplab_platform\mplab_ipe\ipecmd.exe"
$hex     = "$PSScriptRoot\out\pwr-probe-test\default.hex"

& $ipecmd -TPPK4 -P24EP256MC206 -F"$hex" -M -OL
