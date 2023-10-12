# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
import sys
import subprocess

import override_utils

EXEC_ROOT = ("..", "..", "..")
INPUT_LIST_PATHS_ARG = "--input_list_paths="
OUTPUT_LIST_PATHS_ARG = "--output_list_paths="


def FindArgIndex(args, *arg_to_find):
    for arg_idx, arg in enumerate(args):
        if arg.startswith(arg_to_find):
            return arg_idx

    return None


# Add brave/chromium_src overrides to the input file list for remote execution.
def AddBraveChromiumSrcInputs(args):
    outputs_arg_idx = FindArgIndex(args, OUTPUT_LIST_PATHS_ARG)
    if not outputs_arg_idx:
        return False
    inputs_arg_idx = FindArgIndex(args, INPUT_LIST_PATHS_ARG)
    if not inputs_arg_idx:
        return False

    output_list_paths = args[outputs_arg_idx][len(OUTPUT_LIST_PATHS_ARG
                                                  ):].split(",")
    outputs = set()
    for output_list_path in output_list_paths:
        with open(output_list_path, "r") as output_list:
            for line in output_list:
                line = line.strip()
                if line:
                    outputs.add(line)

    input_list_paths = args[inputs_arg_idx][len(INPUT_LIST_PATHS_ARG):].split(
        ",")

    modified_anything = False
    for input_list_idx, input_list_path in enumerate(input_list_paths):
        inputs = []
        did_modify_inputs = False
        with open(input_list_path, "r") as input_list:
            for line in input_list:
                line = line.strip()
                if not line:
                    continue

                if line not in outputs:
                    inputs.append(line)

                if not line.endswith((".mojom", ".py", ".pdl")):
                    continue

                assert line.startswith("src/"), line
                new_input = line.replace("src/", "src/brave/chromium_src/", 1)
                if os.path.exists(os.path.join(*EXEC_ROOT, new_input)):
                    if not modified_anything:
                        inputs.append("src/brave/script/import_inline.py")
                        modified_anything = True
                    inputs.append(new_input)
                    did_modify_inputs = True

        if did_modify_inputs:
            input_list_path += "_remote.rsp"
            with open(input_list_path, "w") as f:
                f.write("\n".join(inputs))
            input_list_paths[input_list_idx] = input_list_path

    if modified_anything:
        args[inputs_arg_idx] = INPUT_LIST_PATHS_ARG + ",".join(input_list_paths)

    return modified_anything


# Add PYTHONPATH env var to remote execution. Paths are relative to the current
# working directory.
def AddRelativePythonPathsToRbe(kwargs):
    abs_exec_root = os.path.abspath(os.path.join(os.getcwd(), *EXEC_ROOT))
    depot_tools_path = os.path.join("brave", "vendor", "depot_tools")
    python_path = []
    for p in sys.path:
        try:
            if not os.path.isabs(p) or not p.startswith(abs_exec_root):
                # We don't care about paths outside of exec_root.
                continue
            if depot_tools_path in p:
                # We don't need depot_tools-specific paths.
                continue
            python_path.append(os.path.relpath(p))
        except ValueError:
            # Path and start are on different drives (Windows-specific).
            pass

    remote_env = os.environ.copy()
    remote_env["PYTHONPATH"] = os.pathsep.join(python_path)

    RBE_env_var_allowlist = remote_env.get("RBE_env_var_allowlist", [])
    if RBE_env_var_allowlist:
        RBE_env_var_allowlist = RBE_env_var_allowlist.split(",")
    RBE_env_var_allowlist.append("PYTHONPATH")
    remote_env["RBE_env_var_allowlist"] = ",".join(RBE_env_var_allowlist)

    kwargs["env"] = remote_env


@override_utils.override_function(subprocess)
def run(original_function, args, **kwargs):
    if AddBraveChromiumSrcInputs(args):
        AddRelativePythonPathsToRbe(kwargs)

    return original_function(args, **kwargs)
