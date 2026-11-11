# SPDX-License-Identifier: MIT
def require($condition; $message):
  if $condition then . else error($message) end;
def names: type == "array" and length > 0 and all(.[]; type == "string" and test("^[A-Za-z0-9+_.-]+$"));
$policy[0] as $p | $copr[0] as $c |
require(($policy|length)==1 and ($copr|length)==1; "Expected one policy and one COPR inventory") |
require($p.schema==1 and ($p.components|type)=="array"; "Unsupported migration policy") |
require(all($p.components[];
  (.id|type)=="string" and (.sources|names) and (.producers|type)=="array" and
  (.destination|IN("kernel", "native-interfaces", "original-userspace", "firmware", "control", "image", "metadata", "build")) and
  (.remaining_work|type)=="string" and (.remaining_work|length)>0 and
  (.original_packages|type)=="array" and .retirement_eligible==false);
  "Incomplete ownership/remaining-work record or unsupported retirement claim") |
require(([$p.components[].id]|length)==([$p.components[].id]|unique|length) and
  ([$p.components[].sources[]]|length)==([$p.components[].sources[]]|unique|length) and
  ([$p.components[].producers[]]|length)==([$p.components[].producers[]]|unique|length);
  "Ambiguous component, source or producer mapping") |
require($c.meta.complete==true and $c.meta.resource=="package" and
  $c.meta.ownername==$p.copr.owner and $c.meta.projectname==$p.copr.project and
  ($c.items|type)=="array" and all($c.items[]; (.id|type)=="number" and .id>0 and .id==(.id|floor) and (.name|type)=="string") and
  ([$c.items[].id]|length)==([$c.items[].id]|unique|length) and
  ([$c.items[].name]|length)==([$c.items[].name]|unique|length);
  "Incomplete, duplicate or wrong-project COPR inventory") |
($rpms|split("\n")|map(select(length>0)|split("\t"))) as $rows |
require(($rows|length)>0 and all($rows[]; length==4 and all(.[]; length>0)); "Malformed or empty RPM inventory") |
($rows|map({name:.[0],evra:.[1],source_rpm:.[2],vendor:.[3]})) as $all |
require(all($all[]; (.name|test("^[A-Za-z0-9+_.-]+$")) and
  (.source_rpm=="(none)" or (.source_rpm|test("^.+-[^-]+-[^-]+[.]src[.]rpm$")))); "Invalid RPM source identity") |
($all|map(. + {source:(if .source_rpm=="(none)" then null else (.source_rpm|capture("^(?<source>.+)-[^-]+-[^-]+[.]src[.]rpm$").source) end)})|
  map(select((.vendor|test("copr";"i")) or (.name|test("^(uke-|senemos-)")) or
    (.source as $source|any($p.components[]; .sources|index($source)))))) as $installed |
([$c.items[]|.name as $name|select(any($p.components[]; .producers|index($name))|not)|$name]|sort) as $unknown_producers |
([$installed[]|.source as $source|select(any($p.components[]; .sources|index($source))|not)|{name,source_rpm}]|unique) as $unknown_packages |
{
  schema:1,
  scope:{owner:$p.copr.owner,project:$p.copr.project,complete_paginated_inventory:true,atomic:false},
  coverage_complete:(($unknown_producers|length)==0 and ($unknown_packages|length)==0),
  producer_count:($c.items|length),
  installed_uke_name_count:([$installed[]|select(.name|startswith("uke-"))|.name]|unique|length),
  unknown_producers:$unknown_producers,unknown_packages:$unknown_packages,
  components:[$p.components[]|. as $component|{
    id,destination,original_packages,remaining_work,retirement_eligible,
    live_producers:[$c.items[]|select(.name as $n|$component.producers|index($n))|{id,name}],
    installed:[$installed[]|select(.source as $s|$component.sources|index($s))|{name,evra,source_rpm}]
  }],
  acceptance:{source_build:false,distribution_signature:false,dnf_transition:false,physical_parity:false},
  note:"Source names and vendor are discovery hints, not signature proof or functional parity. No package, firmware, boot state or producer is changed."
}
