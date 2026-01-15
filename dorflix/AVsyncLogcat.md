# Start basic logcat monitoring

adb logcat

# Monitor with timestamps and continuous output

adb logcat -v time

# Filter logs for Dorflix app only

adb logcat -v time | Select-String -Pattern "com\.dorflix\.app"

# Filter for specific components

adb logcat -v time | Select-String -Pattern "(VideoDecoder|AVSyncController|MediaClock|AVBuffer|SyncLogger)"

# Monitor all sync-related logs

adb logcat -v time | Select-String -Pattern "(SYNC|PERF|BUFFER|CLOCK|QUALITY|SAFE_EXEC)"

# Monitor sync quality changes

adb logcat -v time | Select-String -Pattern "sync_quality|Sync quality changed"

# Monitor performance metrics

adb logcat -v time | Select-String -Pattern "PERFORMANCE REPORT|avg=|calls="

# Monitor for crashes and exceptions

adb logcat -v time | Select-String -Pattern "(CRASH|EXCEPTION|FAILED|ERROR.\*SYNC|std::|AVERROR)"

# Monitor resource cleanup

adb logcat -v time | Select-String -Pattern "(ResourceGuard|cleanup|destroy)"

# Check connected devices

adb devices

# Connect to specific device (if multiple)

adb -s DEVICE_ID logcat

# Clear logs before starting (optional)

adb logcat -c

# Monitor sync quality changes in real-time

adb logcat -v time | Select-String -Pattern "sync*quality|PERFECT|GOOD|POOR|BROKEN" | ForEach-Object {
$timestamp = Get-Date -Format "HH:mm:ss"
    Write-Host "[$timestamp] $*" -ForegroundColor $(if ($_ -match "PERFECT") { "Green" } elseif ($_ -match "BROKEN") { "Red" } else { "Yellow" })
}

# Extract and display performance metrics

adb logcat -v time | Select-String -Pattern "PERFORMANCE REPORT|avg=" | ForEach-Object {
if ($_ -match "PERFORMANCE REPORT") {
Write-Host "`n=== PERFORMANCE REPORT ===" -ForegroundColor Cyan
} else {
Write-Host $_ -ForegroundColor White
}
}

# Alert on crashes with sound/notification

adb logcat -v time | Select-String -Pattern "(FAILED|EXCEPTION|CRASH|std::)" | ForEach-Object {
$timestamp = Get-Date -Format "HH:mm:ss"
    Write-Host "[$timestamp] ALERT: $\_" -ForegroundColor Red
[console]::beep(800, 300) # Audio alert
}
