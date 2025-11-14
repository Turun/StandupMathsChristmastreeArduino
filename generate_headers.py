# use like:
# python3 generate_headers.py /share/Programming/PycharmProjects/StandupMathsChristmasTree/ headers
# to generate required header files
import os
import argparse
import pathlib

def sanitize_identifier(name: str) -> str:
    """Turn filename into a valid C identifier."""
    out = []
    for ch in name:
        if ch.isalnum():
            out.append(ch)
        else:
            out.append('_')
    ident = ''.join(out)
    # Must not start with a digit
    if ident[0].isdigit():
        ident = "_" + ident
    return ident


def create_header(input_path: pathlib.Path, output_path: pathlib.Path):
    content = input_path.read_text(encoding="utf-8")

    identifier = sanitize_identifier(input_path.stem + "_" + input_path.suffix[1:])

    header = (
        f'#pragma once\n\n'
        f'// Auto-generated from: {input_path.name}\n'
        f'const char {identifier}[] PROGMEM = R"rawliteral(\n'
        f'{content}\n'
        f')rawliteral";\n'
    )

    output_path.write_text(header, encoding="utf-8")
    print(f"✓ Generated {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Convert static web files to Arduino PROGMEM .h files.")
    parser.add_argument("input_dir", help="Directory containing .html/.css/.js files")
    parser.add_argument("output_dir", help="Directory where .h files will be written")
    args = parser.parse_args()

    in_dir = pathlib.Path(args.input_dir)
    out_dir = pathlib.Path(args.output_dir)

    if not in_dir.exists():
        print(f"Input directory does not exist: {in_dir}")
        return

    out_dir.mkdir(parents=True, exist_ok=True)

    for root, dirs, files in os.walk(in_dir):
        for file in files:
            if not file.lower().endswith((".html", ".css", ".js")):
                continue

            input_path = pathlib.Path(root) / file

            relative = input_path.relative_to(in_dir)
            output_subdir = out_dir / relative.parent
            output_subdir.mkdir(parents=True, exist_ok=True)

            output_path = output_subdir / (relative.stem + "_" + relative.suffix[1:] + ".h")

            create_header(input_path, output_path)


if __name__ == "__main__":
    main()
