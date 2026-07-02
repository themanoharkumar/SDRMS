# Contributing to SDRMS

Thank you for your interest in contributing to the **Smart Disaster Relief Management System (SDRMS)**! As a project showcasing advanced software engineering practices, clean code conventions and systematic workflows are highly valued.

---

## 🛠️ Development Setup

1. **Fork & Clone:**
   ```bash
   git clone https://github.com/yourusername/SmartDisasterSystem.git
   cd SmartDisasterSystem
   ```
2. **Setup IDE:**
   - Open **Qt Creator**.
   - Select **Open File or Project** and load `CMakeLists.txt`.
   - Choose the **Qt 6.x MinGW 64-bit** kit.
   - Run a test compile to ensure dependencies load correctly.

---

## 📐 Coding Standards

To maintain clean and readable code across the codebase, please adhere to the following rules:

- **Style:** We follow the standard Qt/C++ camelCase style naming convention.
  - Class Names: `CamelCase` (e.g. `DisasterRegistry`, `OverviewPage`)
  - Methods & Variables: `camelCase` (e.g. `registerDisaster()`, `countLabel_`)
  - Member Variables: Suffix with an underscore (e.g. `model_`, `table_`)
- **Formatting:** Keep indentation consistent using **4 spaces** (no tabs).
- **Standards:** Code must be compliant with **C++17** standards.
- **Complexity:** Ensure any new algorithm additions include asymptotic complexity analyses (Big-O notation) in their respective headers.

---

## 🔄 Git Branching Workflow

1. Create a descriptive feature branch:
   ```bash
   git checkout -b feature/your-feature-name
   ```
2. Make modular, logical commits:
   ```bash
   git commit -m "feat: add rescue team allocation logic"
   ```
3. Push to your branch and open a Pull Request (PR) against the `main` branch.

---

## 🧪 Testing and Verification

Before opening a PR:
- Ensure the project builds with **zero compile warnings or errors**.
- Test data persistence: Register several mock items, close and reopen the application, and verify that values load correctly from the SQLite database and local flat-files.
