Add-Type -AssemblyName System.Drawing

$bmp = New-Object System.Drawing.Bitmap 256, 256
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit

# Draw rounded rectangle background
$path = New-Object System.Drawing.Drawing2D.GraphicsPath
$radius = 48
$path.AddArc(8, 8, $radius, $radius, 180, 90)
$path.AddArc(248 - $radius, 8, $radius, $radius, 270, 90)
$path.AddArc(248 - $radius, 248 - $radius, $radius, $radius, 0, 90)
$path.AddArc(8, 248 - $radius, $radius, $radius, 90, 90)
$path.CloseFigure()

$pt1 = New-Object System.Drawing.Point 0, 0
$pt2 = New-Object System.Drawing.Point 256, 256
$c1 = [System.Drawing.Color]::FromArgb(255, 15, 23, 42)
$c2 = [System.Drawing.Color]::FromArgb(255, 30, 41, 59)
$brushBg = New-Object System.Drawing.Drawing2D.LinearGradientBrush -ArgumentList $pt1, $pt2, $c1, $c2
$g.FillPath($brushBg, $path)

# Draw subtle border
$cBorder = [System.Drawing.Color]::FromArgb(180, 51, 65, 85)
$penBorder = New-Object System.Drawing.Pen -ArgumentList $cBorder, 4
$g.DrawPath($penBorder, $path)

# Draw golden lightning
$lPath = New-Object System.Drawing.Drawing2D.GraphicsPath
$pts = @(
    (New-Object System.Drawing.PointF 140, 45),
    (New-Object System.Drawing.PointF 85, 135),
    (New-Object System.Drawing.PointF 130, 135),
    (New-Object System.Drawing.PointF 115, 215),
    (New-Object System.Drawing.PointF 175, 120),
    (New-Object System.Drawing.PointF 135, 120)
)
$lPath.AddPolygon($pts)

$ptL1 = New-Object System.Drawing.Point 85, 45
$ptL2 = New-Object System.Drawing.Point 175, 215
$cL1 = [System.Drawing.Color]::FromArgb(255, 251, 191, 36)
$cL2 = [System.Drawing.Color]::FromArgb(255, 245, 158, 11)
$brushLightning = New-Object System.Drawing.Drawing2D.LinearGradientBrush -ArgumentList $ptL1, $ptL2, $cL1, $cL2
$g.FillPath($brushLightning, $lPath)

# Draw small blue glow dot
$cBlue = [System.Drawing.Color]::FromArgb(255, 37, 99, 235)
$brushBlue = New-Object System.Drawing.SolidBrush -ArgumentList $cBlue
$g.FillEllipse($brushBlue, 170, 55, 24, 24)

$hIcon = $bmp.GetHicon()
$icon = [System.Drawing.Icon]::FromHandle($hIcon)
$fs = New-Object System.IO.FileStream 'D:\schedule-worklog-c\app.ico', ([System.IO.FileMode]::Create)
$icon.Save($fs)
$fs.Close()
$penBorder.Dispose()
$brushBg.Dispose()
$brushLightning.Dispose()
$brushBlue.Dispose()
$g.Dispose()
$bmp.Dispose()
Write-Host "Generated app.ico successfully"
