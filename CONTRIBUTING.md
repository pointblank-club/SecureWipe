# Contributing to SecureWipe

Thank you for your interest in contributing to SecureWipe! This document provides guidelines and instructions for contributing to the project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [How to Contribute](#how-to-contribute)
- [Coding Standards](#coding-standards)
- [Commit Guidelines](#commit-guidelines)
- [Pull Request Process](#pull-request-process)
- [Testing](#testing)
- [Documentation](#documentation)
- [Security Considerations](#security-considerations)
- [License](#license)

## Code of Conduct

By participating in this project, you agree to maintain a respectful and inclusive environment. We expect all contributors to:

- Be respectful and considerate in communication
- Welcome newcomers and help them get started
- Accept constructive criticism gracefully
- Focus on what is best for the community and project

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR-USERNAME/SecureWipe.git
   cd SecureWipe
   ```
3. **Add the upstream remote**:
   ```bash
   git remote add upstream https://github.com/pointblank-club/SecureWipe.git
   ```
4. **Create a branch** for your changes:
   ```bash
   git checkout -b feature/your-feature-name
   ```

## Development Setup

### Prerequisites

- **C++ Compiler**: g++ with C++20 support
- **Build Tools**: make
- **Libraries**:
  - ncurses (ncursesw)
  - libmenu
  - libpanel
  - libform
  - libtinfo

### Building from Source

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential libncurses-dev libncursesw5-dev

# Build the project
make

# Clean build artifacts
make clean
```

### Project Structure

```
SecureWipe/
├── include/          # Header files
│   ├── app/         # Application logic headers
│   ├── core/        # Core functionality headers
│   ├── flow/        # Workflow management headers
│   ├── ui/          # User interface headers
│   └── wipe/        # Wipe algorithm headers
├── src/             # Source files
│   ├── algorithms/  # Wipe algorithms implementation
│   ├── app/         # Application logic
│   ├── core/        # Core functionality
│   ├── flow/        # Workflow management
│   └── ui/          # User interface
├── scripts/         # Shell scripts for various operations
├── build/           # Build artifacts (generated)
└── bin/             # Compiled binaries (generated)
```

## How to Contribute

### Types of Contributions

We welcome various types of contributions:

- **Bug fixes**: Fix issues identified in the issue tracker
- **New features**: Add new wiping algorithms or device support
- **Documentation**: Improve README, code comments, or guides
- **Testing**: Write unit tests or integration tests
- **UI/UX improvements**: Enhance the user interface
- **Performance optimizations**: Improve execution speed or memory usage
- **Security enhancements**: Strengthen security measures

### Finding Issues to Work On

- Browse the [issue tracker](https://github.com/pointblank-club/SecureWipe/issues)
- Look for issues labeled `good first issue` or `help wanted`
- Check the project roadmap for planned features

### Reporting Bugs

Before creating a bug report:

1. Check if the issue already exists in the issue tracker
2. Verify you're using the latest version
3. Test with minimal configuration to reproduce the issue

When reporting a bug, include:

- Clear, descriptive title
- Steps to reproduce the issue
- Expected behavior vs actual behavior
- System information (OS, hardware, SecureWipe version)
- Relevant logs or error messages
- Screenshots if applicable

### Suggesting Features

When suggesting a new feature:

- Check if it's already been suggested
- Clearly describe the feature and its benefits
- Explain use cases and why it's needed
- Consider potential implementation approaches
- Be open to discussion and alternative solutions

## Coding Standards

### C++ Style Guidelines

- Follow **modern C++20** best practices
- Use **meaningful variable and function names**
- Keep functions **small and focused** on a single responsibility
- Use **const correctness** where applicable
- Prefer **RAII** for resource management
- Use **smart pointers** instead of raw pointers when possible

### Code Formatting

- **Indentation**: Use tabs or 4 spaces consistently
- **Braces**: Use K&R or Allman style consistently
- **Line length**: Keep lines under 100 characters when practical
- **Comments**: Write clear, concise comments for complex logic

### Example Code Style

```cpp
// Good: Clear, descriptive function name with proper const
std::vector<DiskInfo> get_available_disks() const {
    std::vector<DiskInfo> disks;
    // Implementation
    return disks;
}

// Good: RAII resource management
void wipe_disk(const std::string& device_path) {
    std::unique_ptr<DiskHandle> disk = open_disk(device_path);
    if (!disk) {
        throw std::runtime_error("Failed to open disk");
    }
    // Disk automatically closed when scope exits
}
```

### Header Files

- Use **include guards** or `#pragma once`
- Keep headers **minimal** and focused
- Forward declare when possible to reduce dependencies
- Document public APIs with clear comments

## Commit Guidelines

### Commit Message Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, no logic change)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Build process or auxiliary tool changes
- `security`: Security improvements

**Examples:**
```
feat(android): Add support for Android 14 devices

Implement detection and wipe support for Android 14
using updated ADB commands and fastboot protocols.

Closes #42
```

```
fix(disk): Correct NVMe device detection on newer kernels

The previous detection logic failed on kernel 6.0+.
Updated to use the correct sysfs paths.

Fixes #38
```

### Commit Best Practices

- Write clear, descriptive commit messages
- Make atomic commits (one logical change per commit)
- Reference relevant issue numbers
- Keep commits focused and small

## Pull Request Process

1. **Update your branch** with the latest upstream changes:
   ```bash
   git fetch upstream
   git rebase upstream/dev
   ```

2. **Ensure your code**:
   - Builds without errors
   - Follows coding standards
   - Includes appropriate tests
   - Updates documentation if needed

3. **Create a pull request**:
   - Use a clear, descriptive title
   - Reference related issues
   - Describe what changes were made and why
   - Include screenshots for UI changes
   - List any breaking changes

4. **PR Review Process**:
   - Maintainers will review your PR
   - Address any requested changes
   - Keep the discussion focused and professional
   - Be patient; reviews may take time

5. **After Approval**:
   - Maintainers will merge your PR
   - Your changes will be included in the next release

### Pull Request Template

```markdown
## Description
Brief description of the changes

## Related Issues
Closes #issue_number

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
Describe the tests you ran and how to reproduce them

## Checklist
- [ ] Code follows project style guidelines
- [ ] Self-review completed
- [ ] Code is well-commented
- [ ] Documentation updated
- [ ] No new warnings generated
- [ ] Tests added/updated
```

## Testing

### Manual Testing

Before submitting a PR:

1. Test the build process: `make clean && make`
2. Test core functionality on supported platforms
3. Verify no regressions in existing features
4. Test edge cases and error handling

### Testing Checklist

- [ ] Builds successfully on Linux
- [ ] UI displays correctly in terminal
- [ ] Disk detection works properly
- [ ] Wipe operations complete successfully
- [ ] Certificates are generated correctly
- [ ] No memory leaks (use valgrind if possible)
- [ ] Error messages are clear and helpful

## Documentation

### Code Documentation

- Document all public APIs and functions
- Explain complex algorithms and logic
- Update README.md for user-facing changes
- Keep documentation in sync with code

### Documentation Style

```cpp
/**
 * Performs secure wipe on the specified disk using ATA Secure Erase.
 *
 * @param device_path Path to the block device (e.g., "/dev/sda")
 * @param verify Enable post-wipe verification
 * @return WipeResult containing status and certificate data
 * @throws DiskAccessException if device cannot be accessed
 */
WipeResult ata_secure_erase(const std::string& device_path, bool verify);
```

## Security Considerations

SecureWipe handles sensitive operations. When contributing:

### Security Guidelines

- **Never disable security checks** without explicit justification
- **Validate all user input** thoroughly
- **Use secure coding practices** to prevent vulnerabilities
- **Test with various disk types** and configurations
- **Document security implications** of changes
- **Report security vulnerabilities** privately to maintainers

### Security Review

Security-sensitive changes require:
- Detailed explanation of security implications
- Testing with edge cases and malicious inputs
- Review by at least one core maintainer
- Documentation of security considerations

## License

By contributing to SecureWipe, you agree that your contributions will be licensed under the same license as the project. Make sure you have the right to contribute the code and that it doesn't violate any third-party licenses.

## Questions?

If you have questions or need help:

- Check existing issues and discussions
- Open a new issue with the `question` label
- Reach out to maintainers

Thank you for contributing to SecureWipe and helping make secure data erasure accessible to everyone!
