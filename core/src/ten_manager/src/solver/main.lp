% The rule for predicate names: Predicate names ending with '_declared' are
% definitely defined by the user (from Clingo's perspective).

%-----------------------------------------------------------------------------
% Declaration
%-----------------------------------------------------------------------------

% do not warn if generated program contains none of these.
% 'error' is intentionally left undefined within the logic program.
#defined error/6.
#defined error/12.

%-----------------------------------------------------------------------------
% Version semantics
%-----------------------------------------------------------------------------

% If a package is selected (selected_pkg == true), then choose at least 1
% versions (selected_pkg_version) from all available versions of that package
% (version_declared).
1 <= { selected_pkg_version(PkgType, PkgName, PkgVersion) : version_declared(PkgType, PkgName, PkgVersion, PkgWeight) }
  :- selected_pkg(PkgType, PkgName).

error(1000, "Select more than 1 version of '[{0}]{1}': '@{2}' and '@{3}'",
      PkgType, PkgName, PkgVersion1, PkgVersion2)
  :- selected_pkg(PkgType, PkgName),
     selected_pkg_version(PkgType, PkgName, PkgVersion1),
     selected_pkg_version(PkgType, PkgName, PkgVersion2),
     PkgVersion1 < PkgVersion2.

error(10, "Select more than 1 version of '[{0}]{1}': '@{2}' introduced by '[{4}]{5}@{6}', and '@{3}' introduced by '[{7}]{8}@{9}'",
      PkgType, PkgName, PkgVersion1, PkgVersion2, IntroducerType1, IntroducerName1, IntroducerVersion1, IntroducerType2, IntroducerName2, IntroducerVersion2)
  :- selected_pkg(PkgType, PkgName),
     selected_pkg_version(PkgType, PkgName, PkgVersion1),
     selected_pkg_version(PkgType, PkgName, PkgVersion2),
     PkgVersion1 < PkgVersion2,
     introducer(PkgType, PkgName, PkgVersion1, IntroducerType1, IntroducerName1, IntroducerVersion1),
     introducer(PkgType, PkgName, PkgVersion2, IntroducerType2, IntroducerName2, IntroducerVersion2).

% Get the weight of the selected version of the selected package. This rule
% declares that only one selected_pkg_weight fact can be derived based on the
% conditions provided.
1 { selected_pkg_weight(PkgType, PkgName, PkgWeight) : version_declared(PkgType, PkgName, PkgVersion, PkgWeight) } 1
  :- selected_pkg_version(PkgType, PkgName, PkgVersion),
     selected_pkg(PkgType, PkgName).

% 'selected_pkg_version' imply new 'selected_pkg'.
selected_pkg(PkgType, PkgName)
  :- selected_pkg_version(PkgType, PkgName, PkgVersion).

%-----------------------------------------------------------------------------
% Tracking Introducers
%-----------------------------------------------------------------------------

% The introducer predicate is used to keep track of how each package version was
% introduced into the selection - either as a root package or as a dependency of
% another package.

introducer(PkgType, PkgName, PkgVersion, IntroducerType, IntroducerName, IntroducerVersion)
  :- depends_on(IntroducerType, IntroducerName, IntroducerVersion, PkgType, PkgName, PkgVersion),
     selected_pkg_version(IntroducerType, IntroducerName, IntroducerVersion),
     selected_pkg_version(PkgType, PkgName, PkgVersion).

% This rule serves to record that a package version is introduced into the
% selection directly as a root package, rather than as a dependency of another
% package. Specifically, it defines an introducer fact for packages that are
% declared as roots.
introducer(PkgType, PkgName, PkgVersion, "root", "", "")
  :- root_declared(PkgType, PkgName),
     selected_pkg_version(PkgType, PkgName, PkgVersion).

%-----------------------------------------------------------------------------
% Dependency semantics
%-----------------------------------------------------------------------------

% Prevent dependencies on undeclared versions.
:- not version_declared(PkgType, PkgName, PkgVersion, _),
   depends_on_declared(_, _, PkgType, PkgName, PkgVersion).

% Select only 1 version for the dependency package.
1 <= { depends_on(PkgType, PkgName, PkgVersion, DepType, DepName, DepVersion) : depends_on_declared(PkgType, PkgName, PkgVersion, DepType, DepName, DepVersion) }
  :- selected_pkg_version(PkgType, PkgName, PkgVersion),
     depends_on_declared(PkgType, PkgName, PkgVersion, DepType, DepName, _).

% Prevent selecting undeclared package versions.
:- selected_pkg_version(PkgType, PkgName, PkgVersion),
   not version_declared(PkgType, PkgName, PkgVersion, _).

% 'depends_on' imply new 'selected_pkg_version'.
selected_pkg_version(DepType, DepName, DepVersion)
  :- depends_on(PkgType, PkgName, _, DepType, DepName, DepVersion).

% Every root_declared must be a selected_pkg.
selected_pkg(PkgType, PkgName) :- root_declared(PkgType, PkgName).

% @{
% Ensure all selected packages are needed (reachable from root).
needed(PkgType, PkgName) :- root_declared(PkgType, PkgName).

needed(DepType, DepName)
  :- needed(PkgType, PkgName),
     depends_on(PkgType, PkgName, _, DepType, DepName, _).

% Constraint to ensure all selected packages are needed.
:- selected_pkg(PkgType, PkgName),
   not needed(PkgType, PkgName).
% @}

%-----------------------------------------------------------------------------
% Optimization
%-----------------------------------------------------------------------------

%
#minimize {
  % What we want to do is, choosing the one with the least weight of the root
  % packages if the total version weight of two subgraphs are equal. In other
  % words, select the highest version of the root packages first.

  % First minimize the weight of packages declared as root packages.
  PkgWeight@998,PkgType,PkgName : root_declared(PkgType, PkgName), selected_pkg_weight(PkgType, PkgName, PkgWeight);

  % Then minimize the weight of all selected packages, regardless of whether
  % they are root packages.
  PkgWeight@999,PkgType,PkgName : selected_pkg_weight(PkgType, PkgName, PkgWeight)
}.

%-----------------------------------------------------------------------------
% Optimization to avoid errors
%-----------------------------------------------------------------------------

% Some errors are handled as rules instead of constraints because it allows us
% to explain why something failed. Here we optimize HEAVILY against the facts
% generated by those rules.

#minimize{ ErrWeight@1000,Msg,Arg1,Arg2,Arg3,Arg4: error(ErrWeight, Msg, Arg1, Arg2, Arg3, Arg4) }.
#minimize{ ErrWeight@1000,Msg,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6,Arg7,Arg8,Arg9,Arg10: error(ErrWeight, Msg, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9, Arg10) }.
