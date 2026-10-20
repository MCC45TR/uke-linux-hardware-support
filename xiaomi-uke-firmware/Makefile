SHELL := /bin/bash
.PHONY: validate srpm
validate:
	jq -e '.schema_version == 1 and .device == "uke" and .soc == "SM7675" and .hardware_tested == false and .python_target_payload == false and (.expected_packages | length > 0)' manifests/component.json >/dev/null
	jq -e '.schema_version==1 and .device=="uke" and (.admitted_files|length)==0 and .firmware_blob_package_ready==false' manifests/firmware-lock.json >/dev/null
	bash -n src/audit-admission.sh
	bash -n src/stage-local.sh
	jq -e '.schema_version==1 and .device=="uke" and .soc=="SM7675" and .public_upload_allowed==false and (.files|length)==74 and all(.files[]; .redistribution_review_complete==false)' manifests/local-firmware-candidates.json >/dev/null
	jq -e '.schema_version==1 and .device=="uke" and .soc=="SM7675" and .source.release=="OS3.0.304.0.WOZMIXM" and .public_upload_allowed==false and .redistribution_review_complete==false and .hardware_tested==false and .python_target_payload==false and (.missing_sensor_configs|sort)==["ak7323.json","sm7675_ak7323.json"] and (.files|length)==143 and all(.files[]; .source_group=="oem" and .redistribution_review_complete==false)' manifests/oem-runtime-source.json >/dev/null
srpm:
	@echo 'No validated Uke package payload yet; complete the manifest acceptance gates.' >&2
	@exit 1
# A lock with no admitted bytes can validate structurally, but never build a
# firmware RPM. There is no metadata-only substitute for a firmware payload.
audit-admission:
	bash src/audit-admission.sh "$(LOCK)" "$(ARCHIVE)" "$(REQUESTS)"
.PHONY: audit-admission test-admission
test-admission: validate
	bash tests/admission.sh
	bash tests/stage-local.sh
.PHONY: stage-local
stage-local: validate
	bash src/stage-local.sh manifests/local-firmware-candidates.json "$(MODEM_FILES)" "$(GPU_FILES)" "$(OUTPUT)"
.PHONY: stage-oem-local
stage-oem-local: validate
	bash src/stage-local.sh manifests/oem-runtime-source.json "$(OEM_FILES)" "$(OUTPUT)"
