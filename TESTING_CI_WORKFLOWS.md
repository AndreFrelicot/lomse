# Testing GitHub Actions CI/CD Workflows

This document provides comprehensive instructions for testing the new GitHub Actions workflows that have been added to the Lomse project.

## Overview

The new CI/CD system includes:
- **Cross-platform builds** (Ubuntu, macOS, Windows)
- **Multiple compiler support** (GCC, Clang, MSVC)
- **Performance optimizations** (ccache, dependency caching)
- **Security analysis** (CodeQL)
- **Memory testing** (Valgrind)
- **Documentation generation** (Doxygen)
- **Release automation**

## Quick Start

### 1. Push Test Branch and Create PR

```bash
# Push the test branch to GitHub
git push -u origin test-ci-workflows

# Create a Pull Request (this triggers the main workflows)
gh pr create --title "test: CI/CD workflow improvements" --body "Testing new comprehensive CI/CD workflows"
```

### 2. Monitor Workflow Execution

1. Navigate to your GitHub repository
2. Click the **"Actions"** tab
3. You should see these workflows running:
   - ✅ `Build and Test` - Main build matrix
   - ✅ `CodeQL Security Analysis` - Security scanning
   - ✅ `Performance and Memory Tests` - Performance testing
   - ✅ `Documentation` - API docs generation

## Detailed Testing Instructions

### Step 1: Validate Workflow Syntax (Local)

Before pushing, ensure workflows are valid:

```bash
# Install yamllint (if not already installed)
brew install yamllint  # macOS
# or
sudo apt-get install yamllint  # Ubuntu

# Validate all workflow files
yamllint .github/workflows/*.yml
```

### Step 2: Test Main Build Workflow

The main `build-PR.yml` workflow will:

**Unix builds (Ubuntu/macOS):**
- Build matrix: 2 OS versions × 2 compilers × 2 build types × 2 library types
- Configurations tested:
  - Ubuntu 22.04/20.04 with GCC/Clang
  - macOS 13/12 with Clang
  - Debug/Release builds
  - Static/Shared libraries

**Windows builds:**
- Build matrix: 2 OS versions × 2 architectures × 1 build type
- Configurations tested:
  - Windows 2022/2019
  - x64/Win32 architectures
  - MSVC toolsets v143/v142

**Expected outcomes:**
- ✅ All platforms build successfully
- ✅ Tests pass on all configurations
- ✅ Build artifacts uploaded
- ✅ Faster builds with ccache (50-80% improvement on subsequent runs)

### Step 3: Test Security Analysis

The `codeql-analysis.yml` workflow will:
- Scan C++ code for security vulnerabilities
- Upload results to GitHub Security tab
- Use enhanced security queries

**To check results:**
1. Go to repository **Security** tab
2. Click **Code scanning**
3. Review any alerts found

### Step 4: Test Performance and Memory Analysis

The `performance.yml` workflow includes three jobs:

**Memory leak detection:**
- Uses Valgrind memcheck
- Custom suppressions for false positives
- Fails build if definite leaks found

**Performance benchmarks:**
- Measures test execution time
- Detects performance regressions
- Sets threshold alerts

**Static analysis:**
- Runs cppcheck for code quality
- Runs clang-tidy on sample files
- Generates analysis reports

### Step 5: Test Documentation Generation

The `documentation.yml` workflow will:
- Generate API documentation with Doxygen
- Create HTML and PDF outputs
- Check documentation quality
- Deploy to GitHub Pages (on master branch)

**To verify:**
1. Check workflow artifacts for HTML docs
2. Look for PDF generation
3. Verify link checking results

### Step 6: Test Release Automation

**⚠️ Only test this when ready for actual release!**

The `release.yml` workflow triggers on version tags:

```bash
# Create and push a test tag
git tag v0.0.1-test
git push origin v0.0.1-test
```

**Expected results:**
- Builds optimized release packages for all platforms
- Creates GitHub release with downloadable assets
- Includes Linux (.tar.gz), macOS (.tar.gz), and Windows (.zip) packages

## Manual Workflow Triggers

You can manually trigger workflows using GitHub CLI:

```bash
# Trigger documentation build
gh workflow run documentation.yml

# Trigger performance tests
gh workflow run performance.yml

# Trigger build workflow
gh workflow run build-PR.yml
```

## Monitoring and Troubleshooting

### Checking Workflow Status

```bash
# List recent workflow runs
gh run list

# View specific workflow run
gh run view <run-id>

# Download artifacts from a run
gh run download <run-id>
```

### Common Issues and Solutions

**1. Dependency Installation Timeouts**
- **Solution:** Retry the workflow run
- **Prevention:** Dependency caching reduces this over time

**2. Windows vcpkg Setup Failures**
- **Symptoms:** Package installation errors on Windows
- **Solution:** Check vcpkg cache, may need to clear and retry
- **Logs:** Look in "Install dependencies (Windows)" step

**3. Test Timeouts**
- **Symptoms:** Tests hang or timeout after 5 minutes
- **Solution:** Check for infinite loops or resource issues
- **Adjustment:** Modify timeout values in workflow if needed

**4. Memory Test False Positives**
- **Symptoms:** Valgrind reports leaks in system libraries
- **Solution:** Update `scripts/valgrind.supp` with new suppressions
- **Format:** Add patterns to suppress known false positives

**5. Build Cache Issues**
- **Symptoms:** Builds not using cache, slower than expected
- **Solution:** Check cache keys, may need to bump cache version
- **Verification:** Look for "Cache hit/miss" messages in logs

### Performance Expectations

**First run (cold cache):**
- Ubuntu: ~15-20 minutes
- macOS: ~20-25 minutes
- Windows: ~25-35 minutes

**Subsequent runs (warm cache):**
- Ubuntu: ~8-12 minutes
- macOS: ~10-15 minutes
- Windows: ~15-20 minutes

**Factors affecting speed:**
- ccache hit rate
- Dependency cache effectiveness
- GitHub runner availability

## Workflow Artifacts

Each workflow produces downloadable artifacts:

**Build artifacts:**
- Compiled libraries (`liblomse*`)
- Test executables (`testlib`)
- Configuration headers (`lomse_config.h`, `lomse_version.h`)

**Test results:**
- Performance reports
- Memory analysis results
- Static analysis reports

**Documentation:**
- HTML documentation
- PDF manual (if generated)
- Quality reports

## Success Criteria

✅ **All workflows complete successfully**
✅ **No test failures across platforms**
✅ **No memory leaks detected**
✅ **Security scan passes**
✅ **Documentation generates cleanly**
✅ **Build times improve on subsequent runs**
✅ **Artifacts upload correctly**

## Next Steps After Successful Testing

1. **Merge the PR:**
   ```bash
   gh pr merge --squash
   ```

2. **Verify master branch workflows:**
   - Documentation auto-deploys to GitHub Pages
   - All workflows run on push to master

3. **Configure branch protection:**
   - Require PR reviews
   - Require status checks to pass
   - Enable automated dependency updates

4. **Set up notifications:**
   - Configure workflow failure alerts
   - Set up Slack/email notifications

## Advanced Configuration

### Customizing Build Matrix

To modify tested configurations, edit the matrix in `.github/workflows/build-PR.yml`:

```yaml
matrix:
  include:
    - os: ubuntu-22.04
      compiler: gcc
      build_type: Release
    # Add more configurations as needed
```

### Adjusting Performance Thresholds

Update performance limits in `performance.yml`:

```yaml
# Adjust timeout for performance regression detection
if (( $(echo "$avg_time > 120" | bc -l) )); then
  echo "Performance regression detected!"
```

### Modifying Memory Test Suppressions

Add patterns to `scripts/valgrind.supp`:

```
{
   my_custom_suppression
   Memcheck:Leak
   match-leak-kinds: definite
   ...
   fun:my_function_pattern
}
```

## Troubleshooting Resources

- **GitHub Actions Documentation:** https://docs.github.com/en/actions
- **Workflow logs:** Available in GitHub Actions tab
- **CMake documentation:** For build configuration issues
- **Valgrind manual:** For memory testing issues

## Contact and Support

If you encounter issues:
1. Check the troubleshooting section above
2. Review workflow logs in GitHub Actions
3. Create an issue with relevant log excerpts
4. Include platform and configuration details

---

*Generated with Claude Code - https://claude.ai/code*