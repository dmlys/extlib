[CmdletBinding()]
Param (
    [parameter(Mandatory=$true)]
    [string]$Path
)

C:\Windows\syswow64\rundll32.exe "C:\Program Files (x86)\Common Files\System\Ole DB\oledb32.dll",OpenDSLFile "$Path"
