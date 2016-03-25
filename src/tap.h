/*
 * The MIT License
 *
 * Copyright (c) 2011 Bruno P. Kinoshita <http://www.kinoshita.eti.br>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @author Bruno P. Kinoshita <http://www.kinoshita.eti.br>
 * @since 0.1
 *
 * @author Sam Spilsbury <s@polysquare.org>
 */

#ifndef TAP_H_
#define TAP_H_

#include <list>
#include <iostream>
#include <map>
#include <fstream>
#include <string>
#include <algorithm>

namespace tap {

namespace mocha {

// based on http://stackoverflow.com/a/7724536/831180
std::string replace_all_copy(
  std::string const& original,
  std::string const& before,
  std::string const& after
) {
  using namespace std;

  if (before == after) return string(original);

  string retval;
  if (before.length() == after.length()) retval.reserve(original.size());

  auto end = original.end();
  auto current = original.begin();
  auto next =
    search(current, end, before.begin(), before.end());

  while ( next != end ) {
    retval.append( current, next );
    retval.append( after );
    current = next + before.size();
    next = search(current, end, before.begin(), before.end());
  }
  retval.append( current, next );
  return retval;
}

class TestResult {

 private:
  int number;
  std::string status;
  std::string name;
  std::string comment;
  bool skip;

 public:
  std::string getComment() const {
    std::stringstream ss;
    if (this->skip) {
      ss << "# SKIP ";
    } else if (!this->comment.empty()) {
      ss << this->comment;
    }
    return ss.str();
  }

  const std::string& getName() const {
    return name;
  }

  int getNumber() const {
    return number;
  }

  const std::string& getStatus() const {
    return status;
  }

  bool getSkip() const {
    return skip;
  }

  void setComment(const std::string& comment) {
    this->comment = comment;
  }

  void setName(const std::string& name) {
    this->name = name;
  }

  void setNumber(int number) {
    this->number = number;
  }

  void setStatus(const std::string& status) {
    this->status = status;
  }

  void setSkip(bool skip) {
    this->skip = skip;
  }

  std::string toString() const {
    std::stringstream ss;
    ss << "    " << this->status << " " << this->number << " " << this->name;
    ss << this->getComment();
    return ss.str();
  }
};

namespace {
  std::string EscapeGTestMessages(std::string const &message) {
    return replace_all_copy(
      replace_all_copy(
        replace_all_copy(
          replace_all_copy(message,
                           "\"", "\\\""),
          "\\n", "[NEWLINE]"),
        "\n", "\\n         "),
      "[NEWLINE]", "\\\\n");
  }

  std::string LeftStrip(std::string input) {
    input.erase(input.begin(),
                std::find_if(input.begin(), input.end(),
                             std::not1(std::ptr_fun<int, int>(std::isspace))));
    return input;
  }
}


class TapListener: public ::testing::EmptyTestEventListener {

 private:
  size_t numTests;
  size_t numFailuresInSuite;
  size_t numSuites;
  std::string currentSuite;

  // Dumps out the current suite's plan and result
  void PrintCurrentSuitePlanAndResult() {
    if (this->numTests) {
      std::cout << "    1.." << this->numTests << std::endl;
      if (this->numFailuresInSuite) {
        std::cout << "not ok "
                  << this->numSuites
                  << " " << this->currentSuite
                  << std::endl;
      } else {
        std::cout << "ok "
                  << this->numSuites
                  << " " << this->currentSuite
                  << std::endl;
      }
    }
  }

public:
  virtual void OnTestEnd(const testing::TestInfo& testInfo) {
    tap::TestResult tapResult;
    tapResult.setName(testInfo.name());
    tapResult.setSkip(!testInfo.should_run());

    if (testInfo.test_case_name() != this->currentSuite) {
      this->PrintCurrentSuitePlanAndResult();

      this->currentSuite = testInfo.test_case_name();
      this->numSuites++;
      this->numTests = 0;
      this->numFailuresInSuite = 0;

      std::cout << "    # Subtest: " << testInfo.test_case_name() << std::endl;
    }

    const testing::TestResult *testResult = testInfo.result();
    int number = testResult->total_part_count();
    if (testResult->Failed() || testResult->HasFatalFailure()) {
      this->numFailuresInSuite++;
      tapResult.setStatus("not ok");
      std::stringstream ss;

      ss << std::endl
         << "    # Diagnostic" << std::endl
         << "      ---" << std::endl;

      for (size_t i = 0; i < number; ++i) {
        auto const &part = testResult->GetTestPartResult(i);
        auto escaped_msg = EscapeGTestMessages(std::string("\n") +
                                               LeftStrip(part.summary()));

        if (part.failed()) {
          ss << "      error: "
             << std::endl
             << "        stack: "
             << (part.file_name() ? part.file_name() : "(unknown)")
             << ":"
             << part.line_number()
             << std::endl
             << "        message: \""
             << escaped_msg << "\""
             << std::endl;
        }
      }

      ss << "      ...";
      tapResult.setComment(ss.str());
    } else {
      tapResult.setStatus("ok");
      double elapsed = (testResult->elapsed_time() / 1000.0);

      std::stringstream timeComment;
      timeComment << " # time=" << elapsed << "s";
      tapResult.setComment(timeComment.str());
    }

    tapResult.setNumber(++numTests);
    std::cout << tapResult.toString() << std::endl;
  }

  virtual void OnTestProgramStart(const testing::UnitTest& unit_test) override {
    //--- Write the count and the word.
    std::cout << "TAP version 13" << std::endl;
    std::cout << "# nesting" << std::endl;
    this->numTests = 0;
    this->numSuites = 0;
    this->numFailuresInSuite = 0;
  }

  virtual void OnTestProgramEnd(const testing::UnitTest& unit_test) {
    //--- Write the count and the word.
    this->PrintCurrentSuitePlanAndResult();
    std::cout << "1.." << numSuites << std::endl;
  }
};

} // namespace mocha

} // namespace tap

#endif // TAP_H_
