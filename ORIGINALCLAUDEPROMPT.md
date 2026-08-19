I’ve switched your effort mode to **xhigh**. Please do a thorough review of the entire codebase before making changes.

### Goal

I want you to review the existing project specifically for **readability, maintainability, consistency, and clarity**, then create a new **Version 6.0** of the project that incorporates the improvements you identify.

Do not focus only on obvious formatting. Look through the codebase carefully and identify places where the code could be easier for another developer—or me in the future—to understand.

### 1. Review the entire codebase

Inspect all relevant source files, modules, classes, functions, configuration files, and existing documentation.

Look for issues such as:

* Poor or inconsistent naming of variables, functions, classes, constants, and files
* Functions or classes that are unnecessarily difficult to understand
* Excessive nesting or complicated control flow
* Repeated or duplicated logic that makes the code harder to maintain
* Inconsistent coding conventions
* Unclear abstractions or separation of responsibilities
* Magic numbers, unexplained constants, or hard-coded values
* Comments that are missing where they would genuinely help understanding
* Comments that are redundant, misleading, outdated, or overly verbose
* Inconsistent formatting or spacing
* Unclear function/class interfaces
* Imports or dependencies that make the structure harder to understand
* Poor organization of modules/files
* Dead, unused, or confusing code where applicable
* Inconsistent error handling
* Ambiguous terminology
* Any other issue that negatively affects readability

Do **not** make changes simply for the sake of changing things. Preserve working behavior and avoid unnecessary refactoring.

### 2. Preserve functionality

The primary purpose of this task is **readability**, not rewriting the project from scratch.

When modifying the code:

* Preserve existing behavior and functionality.
* Avoid introducing unnecessary architectural changes.
* Do not change algorithms unless a change is necessary to make the code substantially clearer or to fix an actual issue discovered during the review.
* Avoid over-engineering.
* Keep the original intent of the project intact.

Where a readability improvement could potentially affect behavior, inspect the relevant code carefully before changing it.

### 3. Create Version 6.0

After reviewing the codebase, create a new version called:

**Version 6.0**

Incorporate the readability improvements directly into this version.

The result should be a coherent, polished version of the project rather than a collection of isolated edits.

Where appropriate, maintain compatibility with the existing project structure and conventions unless those conventions are themselves part of the readability problem.

### 4. Create a new README.md

Create a new, professional `README.md` that explains the **structure and organization of Version 6.0**.

The README should explain, at an appropriate level:

* What the project is
* What Version 6.0 represents
* The overall project structure
* What the major directories/files are responsible for
* How the important components relate to one another
* Where the main logic lives
* How someone unfamiliar with the codebase should navigate it
* Any important setup or usage information that can reasonably be inferred from the existing project
* Any assumptions or configuration that a developer needs to know

Do not invent functionality that does not exist. Base the README on the actual codebase.

### 5. Add author/modification notes

In the README, include a clearly labeled **Author / Modification Notes** section.

State that:

* The original author is **Chris Lock**.
* I am **Jeremy** and am modifying the project.
* The readability and documentation changes in Version 6.0 were made with assistance from **Claude Code**.

Make the wording professional and factual rather than implying that the original project was poorly written.

### 6. Explain the readability changes

The README should also include a section explaining **what was changed in Version 6.0 specifically for readability**.

Summarize the meaningful improvements you actually made, such as:

* Naming improvements
* Code organization
* Simplified control flow
* Documentation/comments
* Refactoring for clarity
* Consistency improvements
* Separation of responsibilities
* Removal of confusing or redundant code

Only mention changes that were actually made.

Where useful, explain the reasoning behind the changes so that the README serves as a record of the cleanup process.

### 7. Be conservative and evidence-based

Before changing anything, understand how the existing code works.

Do not make assumptions about the project merely from filenames or superficial patterns. Trace how important pieces interact before refactoring them.

Prioritize changes that make the code:

**easier to read → easier to navigate → easier to understand → easier to maintain**

rather than simply making it look different.

### 8. Final review

After completing the Version 6.0 changes:

* Review the modified files again for consistency.
* Check for accidental inconsistencies introduced during refactoring.
* Make sure naming conventions are consistent.
* Make sure comments/documentation match the actual code.
* Ensure the README accurately reflects the final structure.
* Look for obvious regressions caused by the readability changes.
* Confirm that the project is internally coherent as Version 6.0.

At the end, provide a concise summary of:

1. The major readability problems you found.
2. The major changes made for Version 6.0.
3. Any changes you intentionally did **not** make and why.
4. The resulting Version 6.0 project structure.
5. Any potential issues or areas that should be reviewed manually.

The goal is a **professional, readable, maintainable Version 6.0**, while preserving the original project's functionality and crediting **Chris Lock as the original author** and **Jeremy + Claude Code as the modifiers of this version**.
