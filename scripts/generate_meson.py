import os

def main(schema_list, include_list, schema_versions):
  with open(os.path.join("static", "redfish", "v1", "schema", "meson.build"), 'w') as meson_file:
    for schema in include_list:
      meson_file.write(f'install_data(\'{schema}_v1.xml\', install_dir: \'share/www/redfish/v1/schema\')\n')

  json_only_schemas = [
      "odata.v4_0_5",
      "odata-v4",
      "redfish-error.v1_0_1",
      "redfish-payload-annotations.v1_2_0",
      "redfish-schema.v1_8_0",
      "redfish-schema-v1",
  ]

  with open(os.path.join("static", "redfish", "v1", "JsonSchemas", "meson.build"), 'w') as meson_file:
    for schema, versions in schema_versions.items():
        if schema in ["RedfishError", "RedfishExtensions", "Validation"]:
          continue
        if schema not in include_list:
          continue

        if versions[-1] != '':
          newest_version = "." + versions[-1]
        else:
          newest_version = ""
        # See https://github.com/DMTF/Redfish/issues/5412
        if schema == "PhysicalContext":
          newest_version = ""

        filename = schema
        if schema.startswith("Oem"):
          filename = "index"

        meson_file.write(f'install_data(\'{schema}{newest_version}.json\', install_dir: \'share/www/redfish/v1/JsonSchemas/{schema}/\', rename: \'{filename}.json\')\n')

    for schema in json_only_schemas:
      filename = "".join(schema.split('.')[0])
      meson_file.write(f'install_data(\'{schema}.json\', install_dir: \'share/www/redfish/v1/JsonSchemas/{filename}/\', rename: \'{filename}.json\')\n')

