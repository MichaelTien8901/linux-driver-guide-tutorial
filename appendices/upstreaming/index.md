---
layout: default
title: "Appendix F: Upstreaming"
nav_order: 22
---

# Appendix F: Upstreaming

Getting your driver into the mainline kernel.

## Workflow Overview

```mermaid
flowchart LR
    A["Write Code"] --> B["checkpatch"]
    B --> C["Format Patches"]
    C --> D["Find Maintainer"]
    D --> E["Send to List"]
    E --> F["Address Feedback"]
    F --> G["Accepted!"]

    style B fill:#738f99,stroke:#0277bd
    style E fill:#7a8f73,stroke:#2e7d32
```

## 1. Check Code Style

```bash
# Run checkpatch
./scripts/checkpatch.pl --strict -f drivers/my_driver/my_driver.c

# Fix warnings before submitting
```

## 2. Write Good Commits

```
subsystem: component: Short summary (< 72 chars)

Longer description explaining:
- What the change does
- Why it's needed
- How it works (if not obvious)

Signed-off-by: Your Name <your.email@example.com>
```

**Rules:**
- Present tense ("Add feature" not "Added feature")
- No period at end of subject
- Blank line between subject and body
- Wrap at 72 characters

## 3. Format Patches

```bash
# Single patch
git format-patch -1 HEAD

# Patch series
git format-patch -v2 --cover-letter -3 HEAD

# Add version notes
git format-patch -v2 --cover-letter -3 HEAD
# Edit 0000-cover-letter.patch
```

## 4. Find Maintainers

```bash
./scripts/get_maintainer.pl drivers/my_driver/my_driver.c

# Output:
# Joe Maintainer <joe@example.org> (maintainer)
# linux-subsystem@vger.kernel.org (open list)
```

## 5. Send Patches

```bash
# Configure git send-email
git config --global sendemail.smtpserver smtp.example.com
git config --global sendemail.smtpserverport 587

# Send
git send-email --to='maintainer@example.org' \
               --cc='linux-kernel@vger.kernel.org' \
               *.patch
```

## 6. Handle Feedback

- Respond promptly (within days)
- Address all comments
- Send new version with `[PATCH v2]`
- Add changelog after `---` in patch

## Common Issues

| Feedback | Fix |
|----------|-----|
| "Style issues" | Run checkpatch, fix all |
| "Missing Signed-off-by" | Add your sign-off |
| "Use devm_*" | Use managed resources |
| "Split into smaller patches" | One logical change per patch |

## Tips

- Subscribe to relevant mailing list first
- Study recent accepted patches for style
- Be patient - reviews take time
- Don't take feedback personally

## Further Reading

- [Submitting Patches](https://docs.kernel.org/process/submitting-patches.html)
- [Coding Style](https://docs.kernel.org/process/coding-style.html)
- [Email Clients](https://docs.kernel.org/process/email-clients.html)
