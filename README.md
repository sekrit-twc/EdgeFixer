EdgeFixer
=========

    ContinuityFixer(clip clip, int "left", int "right", int "top", int "bottom", int "radius")
    ReferenceFixer(clip clip, clip ref, int "left", int "right", int "top", int "bottom", int "radius")
    
    edgefixer.Continuity(clip clip, int "left", int "right", int "top", int "bottom", int "radius")
    edgefixer.Reference(clip clip, clip ref, int "left", int "right", int "top", int "bottom", int "radius")

EdgeFixer repairs bright and dark line artifacts near the border of an image. When an image is resampled with a negative-lobe kernel, such as Bicubic or Lanczos, a series of bright and dark lines may appear around the image borders. These lines need not be cropped, as they contain spatial information that can be recovered. EdgeFixer uses least squares regression to correct the offending lines based on a reference line. ContinuityFixer uses the adjacent line as the reference, whereas ReferenceFixer uses an external reference image.

* **left**, **right**, **top**, **bottom** - the number of lines to filter along each edge
* **radius** - limit the window used for the least squares regression, useful in the presence of overlaid content
