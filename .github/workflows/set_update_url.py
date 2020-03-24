import sys
str = "::set-output name=upload_url::"
with  open("release_url/release_url.txt", "r") as f:
    sys.stdout.write(str + f.read())
