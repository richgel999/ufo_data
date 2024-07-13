import json

def parse_and_write_json(input_file, output_file):
    # Read the JSON file into memory using utf-8-sig to handle BOM
    with open(input_file, 'r', encoding='utf-8-sig') as f:
        data = json.load(f)

    # Extract the "Majestic Timeline" field
    majestic_timeline = data.get("Majestic Timeline", [])

    # Prepare the output content
    output_content = ""
    for idx, entry in enumerate(majestic_timeline):
        basic_date = entry.get("basic_date", "")
        desc = entry.get("desc", "")
        ref = entry.get("ref", "")
        source = entry.get("source", "")
        source_id = entry.get("source_id", "")
        entry_type = entry.get("type", "")
        location = entry.get("location", "")
        time = entry.get("time", "")
        attributes = entry.get("attributes", [])

        # Handle "ref" field which can be a single string or an array of strings
        if isinstance(ref, list):
            ref = "; ".join(ref)
        
        # Handle "location" field which can be a single string or an array of strings
        if isinstance(location, list):
            location = "; ".join(location)
        
        # Handle "attributes" field which can be an array of strings
        if isinstance(attributes, list):
            attributes = "; ".join(attributes)

        output_content += (
            f"** Event {idx} **\n"
            f"Date: {basic_date}\n"
            f"Description: {desc}\n"
            f"Reference: {ref}\n"
            f"Source: {source}\n"
            f"Source ID: {source_id}\n"
        )

        if entry_type:
            output_content += f"Type: {entry_type}\n"
        
        if location:
            output_content += f"Location: {location}\n"
        
        if attributes:
            output_content += f"Attributes: {attributes}\n"
        
        if time:
            output_content += f"Time: {time}\n"
        
        output_content += "\n"

    # Write the output content to a UTF-8 text file
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(output_content)

# Example usage
input_file = 'majestic.json'  # Replace with the path to your input JSON file
output_file = 'majestic.txt'  # Replace with the desired output file path

parse_and_write_json(input_file, output_file)

