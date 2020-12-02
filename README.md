<p align=center>
	<a href="https://www.codacy.com/manual/CHOSTeam/CHicago-Kernel?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=CHOSTeam/CHicago-Kernel&amp;utm_campaign=Badge_Grade"><img src="https://app.codacy.com/project/badge/Grade/7aaacd97d3fb4dc983105849412666ef"></img></a>
</p>

# CHicago Kernel

This is the portable kernel that CHicago uses. Currently, the kernel supports x86 and amd64.

# Known Bugs

The x86/amd64 scheduler is probably broken (for switching between rings 3 and 0), we should probably rewrite it.

# Building

After extracting the toolchain and running the env script (see instructions on the main repository README), you can just run 'build' on the kernel folder, if you want to build just the kernel.
