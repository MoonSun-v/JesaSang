param(
    [string]$mdPath,
    [string]$pptPath
)

$lines = Get-Content -Path $mdPath -Encoding UTF8
$slides = @()
$titleSlide = $null
$current = $null

foreach ($line in $lines) {
    if ($line -match '^#\s+(.*)') {
        $titleSlide = @{ title = $Matches[1]; bullets = @() }
        continue
    }
    if ($line -match '^작성자:') {
        if ($titleSlide) { $titleSlide.bullets += $line }
        continue
    }
    if ($line -match '^범위:') {
        if ($titleSlide) { $titleSlide.bullets += $line }
        continue
    }
    if ($line -match '^##\s+(.*)') {
        if ($titleSlide) { $slides += ,$titleSlide; $titleSlide = $null }
        if ($current) { $slides += ,$current }
        $current = @{ title = $Matches[1]; bullets = @() }
        continue
    }
    if ($line -match '^-\s+(.*)') {
        if ($current) { $current.bullets += $Matches[1] }
        continue
    }
}

if ($titleSlide) { $slides += ,$titleSlide }
if ($current) { $slides += ,$current }

$ppt = New-Object -ComObject PowerPoint.Application
$pres = $ppt.Presentations.Add()

foreach ($s in $slides) {
    $slide = $pres.Slides.Add($pres.Slides.Count + 1, 1)
    $slide.Shapes.Title.TextFrame.TextRange.Text = $s.title
    if ($s.bullets.Count -gt 0) {
        $body = $slide.Shapes.Placeholders.Item(2).TextFrame.TextRange
        $body.Text = $s.bullets[0]
        for ($i = 1; $i -lt $s.bullets.Count; $i++) {
            $null = $body.InsertAfter("`r" + $s.bullets[$i])
        }
    }
}

$pres.SaveAs($pptPath)
$pres.Close()
$ppt.Quit()

