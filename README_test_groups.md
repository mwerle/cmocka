# CMocka - support for multiple suites proposal

## Problem Description

Currently, CMocka only supports running a single test suite at a time. A
test-runner can define and run multiple suites, but each is run in isolation,
and the XML output reflects this:

```xml
<?xml version="1.0" encoding="UTF-8" ?>
<testsuites>
  <testsuite name="tests_A" time="0.000" tests="2" failures="0" errors="0" skipped="0" >
    <testcase name="test_A_ok" time="0.000" >
    </testcase>
    <testcase name="test_A_bad_params" time="0.000" >
    </testcase>
  </testsuite>
</testsuites>
<testsuites>
  <testsuite name="tests_B" time="0.000" tests="2" failures="0" errors="0" skipped="0" >
    <testcase name="test_B_ok" time="0.000" >
    </testcase>
    <testcase name="test_B_bad_params" time="0.000" >
    </testcase>
  </testsuite>
</testsuites>
<testsuites>
  <testsuite name="tests_C" time="0.000" tests="2" failures="0" errors="0" skipped="0" >
    <testcase name="test_C_ok" time="0.000" >
    </testcase>
    <testcase name="test_C_bad_params" time="0.000" >
    </testcase>
  </testsuite>
</testsuites>

```

The XML output in this case _should_ be:
```xml
<?xml version="1.0" encoding="UTF-8" ?>
<testsuites>
  <testsuite name="tests_A" time="0.000" tests="2" failures="0" errors="0" skipped="0" >
    <testcase name="test_A_ok" time="0.000" >
    </testcase>
    <testcase name="test_A_bad_params" time="0.000" >
    </testcase>
  </testsuite>
  <testsuite name="tests_B" time="0.000" tests="2" failures="0" errors="0" skipped="0" >
    <testcase name="test_B_ok" time="0.000" >
    </testcase>
    <testcase name="test_B_bad_params" time="0.000" >
    </testcase>
  </testsuite>
  <testsuite name="tests_C" time="0.000" tests="2" failures="0" errors="0" skipped="0" >
    <testcase name="test_C_ok" time="0.000" >
    </testcase>
    <testcase name="test_C_bad_params" time="0.000" >
    </testcase>
  </testsuite>
</testsuites>

```

Furthermore, the code to run such a set of test suites looks something like
this:

```c
int main(int argc, char **argv)
{
	int ret = OK;

	const struct CMUnitTest tests_A[] = {
		cmocka_unit_test_setup_teardown(test_A_ok, test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_A_bad_params, test_setup, test_teardown),
	};
	const struct CMUnitTest tests_B[] = {
		cmocka_unit_test_setup_teardown(test_B_ok, test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_B_bad_params, test_setup, test_teardown),
	};
	const struct CMUnitTest tests_C[] = {
		cmocka_unit_test_setup_teardown(test_C_ok, test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_C_bad_params, test_setup, test_teardown),
	};

	ChkRet(cmocka_run_group_tests(tests_A, suite_A_init, suite_A_clean));
	ChkRet(cmocka_run_group_tests(tests_B, suite_B_init, suite_B_clean));
	ChkRet(cmocka_run_group_tests(tests_C, suite_C_init, suite_C_clean));

DONE:
	return ret;
}
```

## Proposal

I propose a set of additional functions, types, and macros to facilitate
running multiple test suites with a single invocation. There are multiple
aims here:

1. Generate expected XML output.
1. Allow to specify test suites in individual compilation units, but be run
   by a single runner.
1. Facilitate listing, searching, and filtering not only by individual test
   name, but also by suite.

### Core changes

* Addition of a CMTestSuite type.
* Addition of a function to run a set of test suites.
* Extension of filtering expressions.

#### CMTestSuite type

The following is proposed as a type definition for a `CMTestGroup`. 

(My preference would be to use `CMTestSuite` to be in-line with other unit
testing framworks and terminology, however this seems to fit in better with
the history of `CMocka`.)

```c
struct CMTestGroup {
	const char *name;
	CMUnitTest *unit_tests;
	size_t num_unit_tests;
	CMFixtureFunction group_setup_func;
	CMFixtureFunction group_teardown_func;
	void *initial_state;
};
```

#### Test Suite runner

The following is the proposed interface to run a set of test suites:

```c
#define cmocka_test_group(F_) { #F_, F_, sizeof(F_)/sizeof(F_[0]), NULL, NULL }
#define cmocka_test_group_name(N_, F_) { N_, F_, sizeof(F_)/sizeof(F_[0]), NULL, NULL }
#define cmocka_test_group_setup_teardown(F_, S_, T_) { #F_, F_, sizeof(F_)/sizeof(F_[0]), S_, T_ }
#define cmocka_test_group_name_setup_teardown(N_, F_, S_, T_) { N_, F_, sizeof(F_)/sizeof(F_[0]), S_, T_ }

#ifdef DOXYGEN
 */
int cmocka_run_test_groups(const struct CMTestGroup test_groups[]);
#else
# define cmocka_run_test_groups(test_groups) \
		_cmocka_run_test_groups(test_groups, sizeof(test_groups) / sizeof((test_groups)[0]))
#endif
```

As an example of how this could work:
```c
int main(int argc, char **argv)
{
	const struct CMUnitTest tests_A[] = {
		cmocka_unit_test(test_A_ok),
		cmocka_unit_test(test_A_bad_params),
	};
	const struct CMUnitTest tests_B[] = {
		cmocka_unit_test_setup_teardown(test_B_ok, test_B_setup, test_B_teardown),
		cmocka_unit_test_setup_teardown(test_B_bad_params, test_B_setup, test_B_teardown),
	};
	const struct CMUnitTest tests_C[] = {
		cmocka_unit_test_name("Test C - ok run", test_C_ok),
		cmocka_unit_test_name("Test C - bad parameters"),
	};

	const struct CMTestGroup test_groups[] = {
		cmocka_test_group(tests_A),
		cmocka_test_group_setup_teardown(tests_B, tests_B_setup, tests_B_teardown),
		cmocka_test_group_name("Test Suite C", tests_C),
	}

	size_t tests_failed = cmocka_run_test_groups(test_groups);

	return (tests_failed != 0)? -1 : 0;
}
```

At a high level, `cmocka_run_test_groups()` could be implemented as follows:
```Ruby
	FOR group IN groups:
		IF NOT group IN group_include_filter:
			group.skipped := TRUE
			CONTINUE
		IF group IN group_exclude_filter:
			group.skipped := TRUE
			CONTINUE
		IF group.setup_func:
			group.setup_func()
		FOR test IN group.tests:
			IF NOT test IN test_include_filter:
				test.skipped := TRUE
				CONTINUE
			IF test IN test_exclude_filter:
				test.skipped := TRUE
				CONTINUE
			run_unit_test(test)
		IF group.teardown_func:
			group.teardown_func()
	
	render_output(groups)
```

### Additional changes

* NULL-terminated arrays
* Filtering on groups
* Listing tests

#### NULL-terminated arrays

Using NULL-terminated arrays opens up the ability to define test suites in
multiple compilation units, and combining them before a single call to run
them as this would no longer rely on "macro magic" to set up the arrays.

Example:
```c
int cmocka_run_test_groups(const struct CMTestSuite test_groups[]);
#else
# define cmocka_run_test_groups(test_groups) \
		_cmocka_run_test_groups(test_groups)
#endif


int main(int argc, char **argv)
{
	const struct CMUnitTest tests_A[] = {
		cmocka_unit_test_setup_teardown(test_A_ok, test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_A_bad_params, test_setup, test_teardown),
		NULL,
	};
	const struct CMUnitTest tests_B[] = {
		cmocka_unit_test_setup_teardown(test_B_ok, test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_B_bad_params, test_setup, test_teardown),
		NULL,
	};
	const struct CMUnitTest tests_C[] = {
		cmocka_unit_test_setup_teardown(test_C_ok, test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_C_bad_params, test_setup, test_teardown),
		NULL,
	};

	const struct CMTestGroup test_groups[] = {
		tests A,
		tests B,
		tests_c,
		NULL,
	}

	size_t tests_failed = cmocka_run_test_groups(test_groups);

	return (tests_failed != 0)? -1 : 0;
}
```

#### Filtering on groups

Since we now have the possibility to run multiple test groups with a single
API call, we should extend the filtering syntax to include this.

I propose using `.` as the separator between groups and tests.

For example, include filters could be:
* `"tests_A.*"` -> run all tests in test suite `tests_A`
* `"*.*ok"` -> run all tests in all test suites which finish with "ok" in their
  name.

For backwards compatibility, ommitting a '.' would be equivalent to 
"*.<expression>"; ie, search amongst all test suites for matching tests.
* `"*ok" == "*.*ok"`

#### Listing tests

It would be very useful to be able to list all tests which match the current
filters (by default `"*.*"`).

In addition to the various `cmocka_run` APIs, I propose adding `cmocka_list`
APIs:
* `cmocka_list_test_groups(const CMockaTestGroup *test_groups)`
* 

