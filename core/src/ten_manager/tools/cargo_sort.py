import toml


def sort_dependencies(file_path):
    with open(file_path, "r") as f:
        data = toml.load(f)

    for section in ["dependencies", "dev-dependencies", "build-dependencies"]:
        if section in data:
            data[section] = dict(sorted(data[section].items()))

    with open(file_path, "w") as f:
        toml.dump(data, f)


if __name__ == "__main__":
    sort_dependencies("Cargo.toml")
    print("Dependencies sorted!")
