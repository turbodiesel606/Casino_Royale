$ErrorActionPreference='Stop'
$uri='https://github.com/turbodiesel606/Releases/releases/download/v0.1.0/JobTracker-0.1.0-windows-x64.7z'
$client=[Net.Http.HttpClient]::new()
$client.Timeout=[TimeSpan]::FromSeconds(20)
try {
 $request=[Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::Head,$uri)
 $response=$client.Send($request)
 $result=[ordered]@{TimeUtc=[DateTime]::UtcNow.ToString('o');Method='HEAD';Url=$uri;Status=[int]$response.StatusCode;Reason=$response.ReasonPhrase;Interpretation='HTTP observation only; no installer or download executed'}
 $response.Dispose()
 $request.Dispose()
} catch {
 $result=[ordered]@{TimeUtc=[DateTime]::UtcNow.ToString('o');Method='HEAD';Url=$uri;Status=$null;Error=$_.Exception.Message;Interpretation='Network check NOT_RUN or failed; no inference about remote asset existence'}
} finally {$client.Dispose()}
$result|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $PSScriptRoot 'logs/15-public-head.json') -Encoding utf8
$result|ConvertTo-Json
