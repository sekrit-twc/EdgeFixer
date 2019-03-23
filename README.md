EdgeFixer
=========

    ContinuityFixer(clip clip, int "left", int "top", int "right", int "bottom", int "radius")
    ReferenceFixer(clip clip, clip ref, int "left", int "top", int "right", int "bottom", int "radius")
    
    edgefixer.Continuity(clip clip, int "left", int "top", int "right", int "bottom", int "radius")
    edgefixer.Reference(clip clip, clip ref, int "left", int "top", int "right", int "bottom", int "radius")

EdgeFixer repairs bright and dark line artifacts near the border of an image. When an image is resampled with a negative-lobe kernel, such as Bicubic or Lanczos, a series of bright and dark lines may appear around the image borders. These lines need not be cropped, as they contain spatial information that can be recovered. EdgeFixer uses least squares regression to correct the offending lines based on a reference line. ContinuityFixer uses the adjacent line as the reference, whereas ReferenceFixer uses an external reference image.

* **left**, **right**, **top**, **bottom** - the number of lines to filter along each edge
* **radius** - limit the window used for the least squares regression, useful in the presence of overlaid content

Examples
========
This example image (4x magnification) is taken from a commercial Blu-ray Disc. The use of bicubic image resizing has left an artifact on the outermost row and column. This is easily corrected by using ContinuityFixer to match the brigthness against the next row/column.

    edgefixer.Continuity(clip, top=1, right=1)

Before:

![Before](https://user-images.githubusercontent.com/2678995/45466794-ebc7e900-b6d0-11e8-944a-1cc3ce4cdf60.png)

After:

![After](https://user-images.githubusercontent.com/2678995/45466793-ebc7e900-b6d0-11e8-9b7e-4cc68e17cc7f.png)

If the image contains many bright or dark line artifacts, the nearest artifact-free row may have a substantially different luminance, and thus be unsuitable for ContinuityFixer. This example image (4x magnification) is taken from a commercial DVD. A poor film transfer has left a beating pattern of alternating bright and dark lines on the border. The 10th pixel column has substantially different average brightness than the first column, as can be seen from the image shift.

    edgefixer.Continuity(clip, left=10)

![Before](https://user-images.githubusercontent.com/2678995/45467300-c688aa00-b6d3-11e8-83e2-3b95d7c8f354.png)
![CF](https://user-images.githubusercontent.com/2678995/45467298-c688aa00-b6d3-11e8-9b65-c77809cfa831.png)

In this case, an alternative luminance reference can be applied through ReferenceFixer. Since the bright and dark pixels alternate in the image, a 3-pixel box blur can be used to smooth out the luminance. This successfully reduces the visible pattern when used as the reference image. The blurred image is only used statistically, so the resulting image does not suffer from detail loss.

    ref = std.BoxBlur(hradius=1, hpasses=1, vpasses=0)
    edgefixer.Reference(clip, ref, left=10)

![RF](https://user-images.githubusercontent.com/2678995/45467299-c688aa00-b6d3-11e8-8729-8b0152245841.png)
