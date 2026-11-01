// SPDX-License-Identifier: MIT
#include "client-access.hpp"
#include "fixture.hpp"
#include <adwaita.h>
#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <unistd.h>

namespace c = uke::client;
namespace {
using Callback = std::function<void()>;
using ChoiceSignal = std::pair<const c::Parameter *, GtkWidget *>;
struct ConfirmationSignal {
  GtkWidget *button;
  std::string required;
  bool blocked;
};
void connect_click(GtkWidget *widget, Callback callback) {
  auto *owned = new Callback(std::move(callback));
  g_signal_connect_data(
      widget, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
        (*static_cast<Callback *>(data))();
      }),
      owned,
      +[](gpointer data, GClosure *) { delete static_cast<Callback *>(data); },
      G_CONNECT_DEFAULT);
}
GtkWidget *label(const std::string &text, const char *style = nullptr) {
  auto *w = gtk_label_new(text.c_str());
  gtk_label_set_xalign(GTK_LABEL(w), 0);
  gtk_label_set_wrap(GTK_LABEL(w), true);
  if (style)
    gtk_widget_add_css_class(w, style);
  return w;
}
GtkWidget *box(GtkOrientation direction = GTK_ORIENTATION_VERTICAL,
               int spacing = 12) {
  return gtk_box_new(direction, spacing);
}
void append(GtkWidget *parent, GtkWidget *child) {
  gtk_box_append(GTK_BOX(parent), child);
}
GtkWidget *button(const char *text, Callback callback,
                  const char *style = nullptr) {
  auto *w = gtk_button_new_with_label(text);
  gtk_widget_set_valign(w, GTK_ALIGN_CENTER);
  if (style)
    gtk_widget_add_css_class(w, style);
  connect_click(w, std::move(callback));
  return w;
}
GtkWidget *text_view(const std::string &text, bool mono = false) {
  auto *w = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(w), false);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(w), false);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(w), GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(w), mono);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(w), 12);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(w), 12);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(w), 12);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(w), 12);
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(w)),
                           text.c_str(), -1);
  return w;
}
void margins(GtkWidget *w, int amount) {
  gtk_widget_set_margin_start(w, amount);
  gtk_widget_set_margin_end(w, amount);
  gtk_widget_set_margin_top(w, amount);
  gtk_widget_set_margin_bottom(w, amount);
}
std::string lower(const std::string &source) {
  gchar *value = g_utf8_casefold(source.c_str(), -1);
  std::string result = value;
  g_free(value);
  return result;
}
std::string display_title(const c::Action &action) {
  static const std::map<std::string, std::string> titles{
      {"status", "Device status"},
      {"overview.battery", "Battery"},
      {"charge.mode", "Charging policy"},
      {"charge.limit", "Charge limit"},
      {"desktop.rotation", "Automatic rotation"},
      {"desktop.brightness", "Automatic brightness"},
      {"flashlight.state", "Flashlight"},
      {"flashlight.level", "Flashlight brightness"},
      {"led.color", "Notification light color"},
      {"microphone.select", "Preferred microphone"}};
  auto found = titles.find(action.id);
  return found == titles.end() ? action.title.en : found->second;
}
std::string display_description(const c::Action &action) {
  static const std::map<std::string, std::string> descriptions{
      {"status", "Inspect your device's reported state. Missing providers and "
                 "unknown readings are shown explicitly."},
      {"overview.battery", "Read battery measurements and available health "
                           "information. Missing measurements stay unknown."},
      {"charge.mode", "Choose a charging policy. Preview it first to check "
                      "whether the installed provider can accept the request."},
      {"charge.status", "Read the charging provider's reported state. Saved "
                        "settings and observed behavior are separate."},
      {"flashlight.level",
       "Choose a brightness request for the built-in "
       "flashlight. Read its applied hardware step afterward."},
      {"desktop.rotation", "Request automatic rotation through the original "
                           "desktop provider in your current session."},
      {"desktop.brightness",
       "Request automatic brightness through the original "
       "desktop provider in your current session."}};
  auto found = descriptions.find(action.id);
  if (found != descriptions.end())
    return found->second;
  auto end = action.description.en.find('.');
  if (end != std::string::npos && end < 220)
    return action.description.en.substr(0, end + 1);
  return action.effect == c::Effect::read
             ? "Read reported state from the installed provider. Missing data "
               "is shown explicitly."
             : "Review this request before continuing in a separate terminal. "
               "The provider checks availability and authorization.";
}
std::string field_title(std::string name) {
  static const std::map<std::string, std::string> names{
      {"available", "Provider available"},
      {"physical_acceptance", "Physical hardware acceptance"},
      {"current_ma", "Current (mA)"},
      {"voltage_mv", "Voltage (mV)"},
      {"temperature_c", "Temperature (°C)"},
      {"capacity_percent", "Charge (%)"},
      {"cycles", "Charge cycles"},
      {"thresholds", "Reported charge thresholds"}};
  if (auto found = names.find(name); found != names.end())
    return found->second;
  std::replace(name.begin(), name.end(), '_', ' ');
  if (!name.empty() && name[0] >= 'a' && name[0] <= 'z')
    name[0] -= 'a' - 'A';
  return c::format_output(name, c::Language::english, true);
}
using Observation = std::pair<std::string, std::string>;
void observations(const Json::Value &value, const std::string &path,
                  std::vector<Observation> &rows, unsigned depth = 0) {
  if (rows.size() >= 48)
    return;
  if (value.isObject() && !value.empty() && depth < 6) {
    for (const auto &key : value.getMemberNames()) {
      if (key == "schema")
        continue;
      observations(value[key],
                   path.empty() ? field_title(key)
                                : path + " / " + field_title(key),
                   rows, depth + 1);
    }
    return;
  }
  if (value.isArray() && !value.empty() && depth < 6) {
    for (Json::ArrayIndex index = 0; index < value.size() && rows.size() < 48;
         ++index)
      observations(value[index], path + " / " + std::to_string(index + 1), rows,
                   depth + 1);
    return;
  }
  std::string text;
  if (value.isNull())
    text = "Unknown (not reported)";
  else if (value.isBool())
    text = value.asBool() ? "Yes" : "No";
  else if (value.isString())
    text = value.asString().empty() ? "Empty value reported" : value.asString();
  else if (value.empty() && (value.isArray() || value.isObject()))
    text = "No entries reported";
  else {
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    text = Json::writeString(writer, value);
  }
  text = c::format_output(text, c::Language::english, true);
  if (text.size() > 4096)
    text = text.substr(0, 4096) + "… (see raw observation)";
  rows.emplace_back(path.empty() ? "Observation" : path, std::move(text));
}
struct Section {
  const char *title;
  const char *icon;
  const char *summary;
  std::vector<std::string> ids;
};
const std::vector<Section> sections{
    {"Status",
     "computer-symbolic",
     "A clear view of your device. Readings are collected only when you ask.",
     {"status", "overview.workload", "hardware.status", "firmware.status"}},
    {"Power",
     "battery-symbolic",
     "Battery readings, charging policy and sleep permissions.",
     {"overview.battery", "charge.status", "charge.admission", "charge.plan",
      "charge.mode", "charge.limit", "wake.status", "wake.set",
      "overview.sleep"}},
    {"Display & lights",
     "video-display-symbolic",
     "Display links, desktop controls and the built-in lights.",
     {"overview.display", "display.bits", "desktop.status", "desktop.plasma",
      "desktop.rotation", "desktop.brightness", "flashlight.status",
      "flashlight.state", "flashlight.level", "led.status", "led.color",
      "led.auto"}},
    {"Connections",
     "network-wired-symbolic",
     "USB, wired networking and NFC. Provider availability is always explicit.",
     {"usb.status", "usb.watch", "usb.role", "usb.power", "ethernet.status",
      "ethernet.profile", "nfc.status", "nfc.state", "cdc.session"}},
    {"Audio & sensors",
     "audio-speakers-symbolic",
     "Inspect audio routes, microphones, cameras and available sensor data.",
     {"overview.audio", "microphone.status", "microphone.select",
      "overview.camera", "sensors.list", "sensors.once", "sensors.watch"}},
    {"Updates",
     "software-update-available-symbolic",
     "Inspect package state and preview transactions before making changes.",
     {"channel.status", "overview.updates", "channel.preview", "channel.set",
      "update.preview", "update.apply", "channel.sync-preview",
      "channel.sync"}},
    {"Advanced",
     "applications-engineering-symbolic",
     "All backend actions, including guarded maintenance and private "
     "diagnostics.",
     {}}};
struct Field {
  const c::Parameter *parameter;
  GtkWidget *widget;
  GtkWidget *explanation = nullptr;
};
class App {
  GtkApplication *application_{};
  GtkWidget *window_{};
  GtkWidget *content_{};
  GtkWidget *page_title_{};
  GtkWidget *back_{};
  GtkWidget *sidebar_{};
  GtkWidget *search_{};
  GtkWidget *list_{};
  GtkWidget *output_{};
  GtkWidget *result_status_{};
  GtkWidget *run_{};
  GtkWidget *stop_{};
  GtkWidget *raw_{};
  GtkWidget *review_window_{};
  GtkWidget *review_proceed_{};
  GtkWidget *review_phrase_{};
  std::vector<Field> fields_;
  std::vector<std::string> trail_;
  std::map<std::string, c::Values> drafts_;
  std::map<std::string, c::Result> results_;
  std::unique_ptr<c::Query> query_;
  std::unique_ptr<uke::gtkui::Fixture> fixture_;
  std::optional<uke::gtkui::Sample> simulated_;
  gint64 started_ = 0;
  guint timer_ = 0;
  int section_ = 0;
  const c::Action *current_ = nullptr;
  bool read_only_ = false;
  bool smoke_ = false;
  int smoke_step_ = 0;
  std::string initial_;
  std::string last_read_;
  c::Values last_values_;
  int exit_code_ = 0;

public:
  App(std::string fixture, bool readonly, bool smoke, std::string start)
      : read_only_(readonly), smoke_(smoke), initial_(std::move(start)) {
    if (!fixture.empty())
      fixture_ = std::make_unique<uke::gtkui::Fixture>(fixture);
  }
  ~App() { cancel(); }
  int run(int argc, char **argv) {
    application_ = GTK_APPLICATION(
        adw_application_new("org.uke.Control.Gtk", G_APPLICATION_NON_UNIQUE));
    g_signal_connect(application_, "activate",
                     G_CALLBACK(+[](GtkApplication *, gpointer data) {
                       static_cast<App *>(data)->activate();
                     }),
                     this);
    g_application_run(G_APPLICATION(application_), argc, argv);
    g_object_unref(application_);
    return exit_code_;
  }

private:
  void error(const std::string &message) {
    auto *dialog = adw_alert_dialog_new("Request unavailable", message.c_str());
    adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "close", "Close");
    adw_dialog_present(ADW_DIALOG(dialog), window_);
  }
  void safe(Callback work) {
    try {
      work();
    } catch (const std::exception &e) {
      error(e.what());
    }
  }
  void clear() {
    while (auto *child = gtk_widget_get_first_child(content_))
      gtk_box_remove(GTK_BOX(content_), child);
    fields_.clear();
    output_ = result_status_ = run_ = stop_ = raw_ = nullptr;
  }
  void activate() {
    window_ = adw_application_window_new(application_);
    gtk_window_set_title(GTK_WINDOW(window_), "Uke Control");
    gtk_window_set_default_size(GTK_WINDOW(window_), 1120, 800);
    gtk_widget_set_size_request(window_, 700, 500);
    auto *root = box(GTK_ORIENTATION_VERTICAL, 0);
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(window_), root);
    auto *header = adw_header_bar_new();
    page_title_ = adw_window_title_new("Uke Control", "Device settings");
    adw_header_bar_set_title_widget(ADW_HEADER_BAR(header), page_title_);
    append(root, header);
    back_ = gtk_button_new_from_icon_name("go-previous-symbolic");
    gtk_widget_set_tooltip_text(back_,
                                "Back — cancels the current observation");
    connect_click(back_, [this] { go_back(); });
    adw_header_bar_pack_start(ADW_HEADER_BAR(header), back_);
    auto *about = button("About", [this] { show_about(); });
    adw_header_bar_pack_end(ADW_HEADER_BAR(header), about);
    if (fixture_ || read_only_) {
      auto *banner =
          adw_banner_new(fixture_ ? "SIMULATION · Synthetic observations · All "
                                    "commands and changes are disabled"
                                  : "READ ONLY · Changes are disabled");
      adw_banner_set_revealed(ADW_BANNER(banner), true);
      append(root, banner);
    }
    auto *split = adw_overlay_split_view_new();
    gtk_widget_set_vexpand(split, true);
    adw_overlay_split_view_set_min_sidebar_width(ADW_OVERLAY_SPLIT_VIEW(split),
                                                 245);
    adw_overlay_split_view_set_max_sidebar_width(ADW_OVERLAY_SPLIT_VIEW(split),
                                                 260);
    append(root, split);
    auto *sidebar_box = box(GTK_ORIENTATION_VERTICAL, 8);
    margins(sidebar_box, 12);
    gtk_widget_add_css_class(sidebar_box, "navigation-sidebar");
    sidebar_ = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(sidebar_),
                                    GTK_SELECTION_SINGLE);
    gtk_widget_add_css_class(sidebar_, "navigation-sidebar");
    append(sidebar_box, sidebar_);
    for (const auto &s : sections) {
      auto *row = gtk_list_box_row_new();
      auto *line = box(GTK_ORIENTATION_HORIZONTAL, 12);
      margins(line, 12);
      append(line, gtk_image_new_from_icon_name(s.icon));
      append(line, label(s.title));
      gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), line);
      gtk_list_box_append(GTK_LIST_BOX(sidebar_), row);
    }
    g_signal_connect(
        sidebar_, "row-selected",
        G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
          if (row) {
            auto *self = static_cast<App *>(data);
            self->section_ = gtk_list_box_row_get_index(row);
            self->show_section();
          }
        }),
        this);
    auto *note = label("Device support depends on the installed providers. "
                       "Unknown readings are never treated as zero.",
                       "dim-label");
    gtk_widget_set_vexpand(note, true);
    gtk_widget_set_valign(note, GTK_ALIGN_END);
    margins(note, 8);
    append(sidebar_box, note);
    adw_overlay_split_view_set_sidebar(ADW_OVERLAY_SPLIT_VIEW(split),
                                       sidebar_box);
    auto *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    auto *clamp = adw_clamp_new();
    adw_clamp_set_maximum_size(ADW_CLAMP(clamp), 900);
    adw_clamp_set_tightening_threshold(ADW_CLAMP(clamp), 650);
    content_ = box();
    margins(content_, 26);
    adw_clamp_set_child(ADW_CLAMP(clamp), content_);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), clamp);
    adw_overlay_split_view_set_content(ADW_OVERLAY_SPLIT_VIEW(split), scroll);
    auto *toggle = gtk_toggle_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), true);
    gtk_button_set_icon_name(GTK_BUTTON(toggle), "sidebar-show-symbolic");
    gtk_widget_set_tooltip_text(toggle, "Show navigation");
    g_object_bind_property(toggle, "active", split, "show-sidebar",
                           static_cast<GBindingFlags>(G_BINDING_BIDIRECTIONAL |
                                                      G_BINDING_SYNC_CREATE));
    adw_header_bar_pack_start(ADW_HEADER_BAR(header), toggle);
    auto *breakpoint =
        adw_breakpoint_new(adw_breakpoint_condition_parse("max-width: 800sp"));
    GValue collapsed = G_VALUE_INIT;
    g_value_init(&collapsed, G_TYPE_BOOLEAN);
    g_value_set_boolean(&collapsed, true);
    adw_breakpoint_add_setter(breakpoint, G_OBJECT(split), "collapsed",
                              &collapsed);
    g_value_set_boolean(&collapsed, false);
    adw_breakpoint_add_setter(breakpoint, G_OBJECT(split), "show-sidebar",
                              &collapsed);
    g_value_unset(&collapsed);
    adw_application_window_add_breakpoint(ADW_APPLICATION_WINDOW(window_),
                                          breakpoint);
    auto *keys = gtk_event_controller_key_new();
    g_signal_connect(
        keys, "key-pressed",
        G_CALLBACK(+[](GtkEventControllerKey *, guint key, guint,
                       GdkModifierType modifiers, gpointer data) -> gboolean {
          auto *self = static_cast<App *>(data);
          if (key == GDK_KEY_Escape) {
            self->go_back();
            return true;
          }
          if (key == GDK_KEY_q && (modifiers & GDK_CONTROL_MASK)) {
            self->cancel();
            gtk_window_close(GTK_WINDOW(self->window_));
            return true;
          }
          return false;
        }),
        this);
    gtk_widget_add_controller(window_, keys);
    g_signal_connect(window_, "close-request",
                     G_CALLBACK(+[](GtkWindow *, gpointer data) -> gboolean {
                       auto *self = static_cast<App *>(data);
                       self->cancel();
                       self->run_ = self->stop_ = self->output_ = nullptr;
                       return false;
                     }),
                     this);
    gtk_list_box_select_row(
        GTK_LIST_BOX(sidebar_),
        gtk_list_box_get_row_at_index(GTK_LIST_BOX(sidebar_), 0));
    if (!initial_.empty())
      safe([this] {
        int target = 6;
        for (std::size_t index = 0; index < sections.size() - 1; ++index)
          if (std::find(sections[index].ids.begin(), sections[index].ids.end(),
                        initial_) != sections[index].ids.end()) {
            target = static_cast<int>(index);
            break;
          }
        gtk_list_box_select_row(
            GTK_LIST_BOX(sidebar_),
            gtk_list_box_get_row_at_index(GTK_LIST_BOX(sidebar_), target));
        open(initial_);
      });
    gtk_window_present(GTK_WINDOW(window_));
    if (smoke_)
      g_timeout_add(
          150,
          +[](gpointer data) -> gboolean {
            return static_cast<App *>(data)->smoke_tick();
          },
          this);
  }
  void show_about() {
    auto *w = adw_about_dialog_new();
    adw_about_dialog_set_application_name(ADW_ABOUT_DIALOG(w), "Uke Control");
    adw_about_dialog_set_application_icon(ADW_ABOUT_DIALOG(w),
                                          "org.uke.Control.Gtk");
    adw_about_dialog_set_version(ADW_ABOUT_DIALOG(w), "1.0.0");
    adw_about_dialog_set_comments(
        ADW_ABOUT_DIALOG(w),
        "Native GTK 4 and libadwaita frontend for the shared Uke Control "
        "client. Driver readback and a successful command do not establish "
        "physical hardware acceptance.");
    adw_about_dialog_set_license_type(ADW_ABOUT_DIALOG(w), GTK_LICENSE_MIT_X11);
    adw_dialog_present(ADW_DIALOG(w), window_);
  }
  void show_section() {
    cancel();
    current_ = nullptr;
    trail_.clear();
    clear();
    gtk_widget_set_sensitive(back_, false);
    adw_window_title_set_subtitle(ADW_WINDOW_TITLE(page_title_),
                                  sections[section_].title);
    append(content_, label(sections[section_].title, "title-1"));
    append(content_, label(sections[section_].summary, "dim-label"));
    if (section_ == 0) {
      auto *card = box();
      gtk_widget_add_css_class(card, "card");
      auto *hero = box();
      margins(hero, 18);
      append(card, hero);
      append(hero, label("Your device, observed", "title-2"));
      append(
          hero,
          label(
              results_.contains("status")
                  ? "A previous observation is available. Read again when you "
                    "need a fresh snapshot."
                  : "No observation has been requested. Read the current "
                    "state to see available providers, reported values and "
                    "missing data."));
      auto *read = button(
          "Read device status",
          [this] {
            safe([this] {
              open("status");
              start_read();
            });
          },
          "suggested-action");
      gtk_widget_set_halign(read, GTK_ALIGN_START);
      append(hero, read);
      append(content_, card);
    }
    search_ = gtk_search_entry_new();
    gtk_widget_set_tooltip_text(search_, section_ == 6 ? "Search all actions"
                                                       : "Filter this page");
    gtk_editable_set_text(GTK_EDITABLE(search_), "");
    append(content_, search_);
    g_signal_connect(search_, "search-changed",
                     G_CALLBACK(+[](GtkSearchEntry *, gpointer data) {
                       static_cast<App *>(data)->populate();
                     }),
                     this);
    list_ = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_), GTK_SELECTION_NONE);
    gtk_widget_add_css_class(list_, "boxed-list");
    append(content_, list_);
    populate();
  }
  void populate() {
    while (auto *child = gtk_widget_get_first_child(list_))
      gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    std::string filter = lower(gtk_editable_get_text(GTK_EDITABLE(search_)));
    unsigned count = 0;
    std::vector<const c::Action *> ordered;
    if (section_ == 6) {
      for (const auto &action : c::actions())
        ordered.push_back(&action);
    } else {
      for (const auto &id : sections[section_].ids)
        ordered.push_back(&c::action(id));
    }
    for (const auto *item : ordered) {
      const auto &a = *item;
      if (!filter.empty() &&
          lower(a.title.en + " " + a.description.en + " " + a.id)
                  .find(filter) == std::string::npos)
        continue;
      auto *row = adw_action_row_new();
      adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row),
                                    display_title(a).c_str());
      std::string sub =
          a.effect == c::Effect::read ? "Observation" : "Review a change";
      if (a.presentation == c::Presentation::terminal)
        sub += " · Terminal";
      if (auto found = results_.find(a.id); found != results_.end())
        sub += found->second.exit_status == 0 ? " · Last read completed"
                                              : " · Last read needs attention";
      adw_action_row_set_subtitle(ADW_ACTION_ROW(row), sub.c_str());
      adw_action_row_add_prefix(
          ADW_ACTION_ROW(row),
          gtk_image_new_from_icon_name(a.effect == c::Effect::read
                                           ? "view-reveal-symbolic"
                                           : "preferences-system-symbolic"));
      auto *arrow =
          button("Open", [this, id = a.id] { safe([this, id] { open(id); }); });
      adw_action_row_add_suffix(ADW_ACTION_ROW(row), arrow);
      adw_action_row_set_activatable_widget(ADW_ACTION_ROW(row), arrow);
      gtk_list_box_append(GTK_LIST_BOX(list_), row);
      ++count;
    }
    if (!count) {
      auto *row = adw_action_row_new();
      adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row),
                                    "No matching actions");
      gtk_list_box_append(GTK_LIST_BOX(list_), row);
    }
  }
  void save_draft() {
    if (current_)
      drafts_[current_->id] = values();
  }
  void go_back() {
    if (!current_)
      return;
    save_draft();
    cancel();
    if (!trail_.empty()) {
      auto id = trail_.back();
      trail_.pop_back();
      open(id, false);
    } else
      show_section();
  }
  c::Values values() const {
    c::Values result;
    for (const auto &field : fields_) {
      const auto &p = *field.parameter;
      if (p.kind == c::Kind::choice) {
        unsigned index =
            gtk_drop_down_get_selected(GTK_DROP_DOWN(field.widget));
        result[p.id] = index < p.choices.size() ? p.choices[index].value : "";
      } else
        result[p.id] = gtk_editable_get_text(GTK_EDITABLE(field.widget));
    }
    return result;
  }
  void open(const std::string &id, bool push = false) {
    if (current_) {
      save_draft();
      if (push)
        trail_.push_back(current_->id);
    }
    cancel();
    current_ = &c::action(id);
    clear();
    const auto &a = *current_;
    gtk_widget_set_sensitive(back_, true);
    adw_window_title_set_subtitle(ADW_WINDOW_TITLE(page_title_),
                                  display_title(a).c_str());
    append(content_, label(display_title(a), "title-1"));
    append(content_, label(display_description(a)));
    auto *details = gtk_expander_new("Technical details and limitations");
    auto *detail_body = box();
    append(detail_body, label(a.description.en));
    append(detail_body,
           label("Action: " + a.id + " · Access: " + c::access_name(a.access) +
                     " · Effect: " + c::effect_name(a.effect),
                 "dim-label"));
    gtk_expander_set_child(GTK_EXPANDER(details), detail_body);
    append(content_, details);
    auto *context = box(GTK_ORIENTATION_HORIZONTAL, 8);
    append(context, label(a.effect == c::Effect::read
                              ? "OBSERVATION"
                              : "REQUEST · Separate confirmation required",
                          "caption-heading"));
    append(content_, context);
    auto draft = drafts_.contains(id) ? drafts_.at(id) : c::defaults(a);
    if (!a.parameters.empty()) {
      auto *group = adw_preferences_group_new();
      adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group),
                                      a.effect == c::Effect::read
                                          ? "Observation options"
                                          : "Requested settings");
      adw_preferences_group_set_description(
          ADW_PREFERENCES_GROUP(group),
          "Form defaults are requests, not current device readings.");
      append(content_, group);
      for (const auto &p : a.parameters) {
        auto *row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row),
                                      p.title.en.c_str());
        adw_action_row_set_subtitle(ADW_ACTION_ROW(row),
                                    p.description.en.c_str());
        adw_action_row_set_subtitle_lines(ADW_ACTION_ROW(row), 0);
        GtkWidget *input = nullptr;
        GtkWidget *explanation = nullptr;
        if (p.kind == c::Kind::choice) {
          auto *strings = gtk_string_list_new(nullptr);
          unsigned chosen = 0, index = 0;
          for (const auto &option : p.choices) {
            gtk_string_list_append(strings, option.title.en.c_str());
            if (option.value == draft[p.id])
              chosen = index;
            ++index;
          }
          input = gtk_drop_down_new(G_LIST_MODEL(strings), nullptr);
          gtk_drop_down_set_selected(GTK_DROP_DOWN(input), chosen);
          gtk_widget_set_size_request(input, 200, -1);
          explanation =
              label(chosen < p.choices.size() ? p.choices[chosen].description.en
                                              : "");
          gtk_widget_add_css_class(explanation, "dim-label");
          margins(explanation, 8);
        } else if (p.kind == c::Kind::integer) {
          input = gtk_spin_button_new_with_range(p.minimum, p.maximum, 1);
          try {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(input),
                                      std::stod(draft[p.id]));
          } catch (...) {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(input), p.minimum);
          }
          gtk_widget_set_tooltip_text(input,
                                      ("Range: " + std::to_string(p.minimum) +
                                       "–" + std::to_string(p.maximum))
                                          .c_str());
        } else {
          input = gtk_entry_new();
          gtk_editable_set_text(GTK_EDITABLE(input), draft[p.id].c_str());
          gtk_entry_set_placeholder_text(GTK_ENTRY(input),
                                         p.optional ? "Optional" : "Required");
          gtk_widget_set_size_request(input, 210, -1);
        }
        gtk_widget_set_valign(input, GTK_ALIGN_CENTER);
        adw_action_row_add_suffix(ADW_ACTION_ROW(row), input);
        adw_action_row_set_activatable_widget(ADW_ACTION_ROW(row), input);
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), row);
        fields_.push_back({&p, input, explanation});
        if (explanation) {
          adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), explanation);
          auto *field_data = new ChoiceSignal(&p, explanation);
          g_signal_connect_data(
              input, "notify::selected",
              G_CALLBACK(+[](GObject *object, GParamSpec *, gpointer data) {
                auto *pair = static_cast<ChoiceSignal *>(data);
                unsigned selected =
                    gtk_drop_down_get_selected(GTK_DROP_DOWN(object));
                gtk_label_set_text(
                    GTK_LABEL(pair->second),
                    selected < pair->first->choices.size()
                        ? pair->first->choices[selected].description.en.c_str()
                        : "");
              }),
              field_data,
              +[](gpointer data, GClosure *) {
                delete static_cast<ChoiceSignal *>(data);
              },
              G_CONNECT_DEFAULT);
        }
      }
    }
    auto *controls = box(GTK_ORIENTATION_HORIZONTAL, 8);
    append(content_, controls);
    run_ = button(
        a.effect == c::Effect::read
            ? (a.presentation == c::Presentation::capture
                   ? "Read current state"
                   : "Review terminal observation")
            : "Review request…",
        [this] {
          safe([this] {
            if (current_->effect == c::Effect::read &&
                current_->presentation == c::Presentation::capture)
              start_read();
            else
              review();
          });
        },
        "suggested-action");
    append(controls, run_);
    if (a.effect != c::Effect::read && (fixture_ || read_only_)) {
      append(content_,
             label(fixture_
                       ? "Simulation blocks all changes. You can still edit "
                         "and review requests and inspect simulated state."
                       : "Read-only mode permits review; terminal continuation "
                         "is disabled.",
                   "warning"));
    }
    stop_ = button("Stop reading", [this] { cancel(true); });
    gtk_widget_set_visible(stop_, false);
    append(controls, stop_);
    if (!a.inspect.empty())
      append(controls,
             button("Inspect current state", [this, target = a.inspect] {
               safe([this, target] { open_related(target); });
             }));
    if (!a.preview.empty())
      append(controls, button("Preview request", [this, target = a.preview] {
               safe([this, target] { open_related(target); });
             }));
    result_status_ =
        label(a.effect == c::Effect::read
                  ? "No observation yet. Select Read to collect current data."
                  : "Request not submitted. Inspect current state and preview "
                    "the request first.",
              "dim-label");
    append(content_, result_status_);
    output_ = box();
    append(content_, output_);
    if (auto found = results_.find(id); found != results_.end())
      render(found->second);
  }
  void open_related(const std::string &target) {
    auto source = values();
    auto destination = c::defaults(c::action(target));
    for (auto &[key, value] : destination)
      if (source.contains(key))
        value = source[key];
    drafts_[target] = std::move(destination);
    open(target, true);
  }
  void cancel(bool display = false) {
    if (timer_) {
      g_source_remove(timer_);
      timer_ = 0;
    }
    if (query_) {
      query_->cancel();
      if (display) {
        auto result = query_->result();
        result.cancelled = true;
        result.running = false;
        results_[result.action_id] = result;
        if (output_)
          render(result);
      }
      query_.reset();
    }
    if (simulated_) {
      if (display) {
        auto result = simulated_->result;
        result.cancelled = true;
        result.running = false;
        result.exit_status = -1;
        result.output.clear();
        result.error = "Simulated observation cancelled before completion.";
        results_[result.action_id] = result;
        if (output_)
          render(result);
      }
      simulated_.reset();
    }
    if (run_)
      gtk_widget_set_sensitive(run_, true);
    if (stop_)
      gtk_widget_set_visible(stop_, false);
  }
  void start_read() {
    if (!current_ || current_->effect != c::Effect::read ||
        current_->presentation != c::Presentation::capture)
      throw std::runtime_error("Only captured observations can run here");
    auto plan = c::plan(current_->id, values());
    cancel();
    last_read_ = current_->id;
    last_values_ = plan.values;
    started_ = g_get_monotonic_time();
    if (fixture_)
      simulated_ = fixture_->sample(current_->id);
    else
      query_ = std::make_unique<c::Query>(current_->id, plan.values);
    gtk_label_set_text(GTK_LABEL(result_status_),
                       fixture_ ? "Reading synthetic observation…"
                                : "Reading current state…");
    gtk_widget_set_sensitive(run_, false);
    gtk_widget_set_visible(stop_, true);
    timer_ = g_timeout_add(
        50,
        +[](gpointer data) -> gboolean {
          return static_cast<App *>(data)->poll();
        },
        this);
  }
  gboolean poll() {
    c::Result result;
    if (query_) {
      query_->poll();
      result = query_->result();
      if (result.running)
        return G_SOURCE_CONTINUE;
    } else if (simulated_) {
      if ((g_get_monotonic_time() - started_) / 1000 < simulated_->delay_ms)
        return G_SOURCE_CONTINUE;
      result = simulated_->result;
      result.elapsed_ms = (g_get_monotonic_time() - started_) / 1000;
    } else {
      timer_ = 0;
      return G_SOURCE_REMOVE;
    }
    timer_ = 0;
    query_.reset();
    simulated_.reset();
    results_[result.action_id] = result;
    render(result);
    gtk_widget_set_sensitive(run_, true);
    gtk_widget_set_visible(stop_, false);
    return G_SOURCE_REMOVE;
  }
  void render(const c::Result &result) {
    while (auto *child = gtk_widget_get_first_child(output_))
      gtk_box_remove(GTK_BOX(output_), child);
    std::string status = result.cancelled   ? "Cancelled"
                         : result.timed_out ? "Timed out"
                         : result.exit_status == 0
                             ? "Observation completed"
                             : "Observation unavailable or failed";
    Json::Value parsed;
    Json::CharReaderBuilder reader_builder;
    std::istringstream input(result.output);
    std::string parse_error;
    const bool parsed_ok =
        Json::parseFromStream(reader_builder, input, &parsed, &parse_error);
    if (parsed_ok && parsed.isObject() && parsed["available"].isBool() &&
        !parsed["available"].asBool() && !result.cancelled && !result.timed_out)
      status = "Provider unavailable · Observation completed";
    status += " · Exit " + std::to_string(result.exit_status) + " · " +
              std::to_string(result.elapsed_ms) + " ms";
    if (result.truncated)
      status += " · Partial output (capture limit reached)";
    if (fixture_)
      status += " · SIMULATION";
    gtk_label_set_text(GTK_LABEL(result_status_), status.c_str());
    gtk_widget_remove_css_class(result_status_, "error");
    if (result.exit_status != 0 || result.timed_out)
      gtk_widget_add_css_class(result_status_, "error");
    if (!result.output.empty()) {
      if (parsed_ok && (parsed.isObject() || parsed.isArray())) {
        auto *list = gtk_list_box_new();
        gtk_list_box_set_selection_mode(GTK_LIST_BOX(list), GTK_SELECTION_NONE);
        gtk_widget_add_css_class(list, "boxed-list");
        std::vector<Observation> rows;
        observations(parsed, "", rows);
        for (const auto &[name, value] : rows) {
          auto *row = adw_action_row_new();
          adw_preferences_row_set_use_markup(ADW_PREFERENCES_ROW(row), false);
          adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), name.c_str());
          adw_action_row_set_subtitle(ADW_ACTION_ROW(row), value.c_str());
          adw_action_row_set_subtitle_lines(ADW_ACTION_ROW(row), 0);
          gtk_list_box_append(GTK_LIST_BOX(list), row);
        }
        append(output_, list);
        if (rows.size() == 48)
          append(output_, label("Showing the first 48 values. The raw "
                                "observation retains the full captured result.",
                                "dim-label"));
      } else {
        auto *frame = gtk_frame_new(nullptr);
        gtk_frame_set_child(GTK_FRAME(frame),
                            text_view(c::format_output(
                                result.output, c::Language::english, false)));
        append(output_, frame);
      }
      auto *raw = gtk_expander_new("Raw observation (JSON / provider output)");
      gtk_expander_set_child(
          GTK_EXPANDER(raw),
          text_view(c::format_output(result.output, c::Language::english, true),
                    true));
      append(output_, raw);
    } else
      append(output_, label("No observation data was returned. This is not a "
                            "zero or a successful device test.",
                            "dim-label"));
    if (!result.error.empty()) {
      append(output_, label("Provider / execution error", "heading"));
      auto *frame = gtk_frame_new(nullptr);
      gtk_frame_set_child(
          GTK_FRAME(frame),
          text_view(c::format_output(result.error, c::Language::english, true),
                    true));
      append(output_, frame);
    }
    auto *refresh = button("Refresh observation", [this] {
      safe([this] {
        if (current_ && current_->id == last_read_) {
          drafts_[last_read_] = last_values_;
          start_read();
        } else
          start_read();
      });
    });
    append(output_, refresh);
  }
  void review() {
    auto plan = c::plan(current_->id, values());
    const bool blocked = fixture_ || read_only_;
    auto *dialog = gtk_window_new();
    review_window_ = dialog;
    gtk_window_set_title(GTK_WINDOW(dialog), "Review request");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window_));
    gtk_window_set_modal(GTK_WINDOW(dialog), true);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), true);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 640, 560);
    auto *header = adw_header_bar_new();
    adw_header_bar_set_title_widget(ADW_HEADER_BAR(header),
                                    label("Review request", "heading"));
    gtk_window_set_titlebar(GTK_WINDOW(dialog), header);
    auto *scroll = gtk_scrolled_window_new();
    auto *body = box();
    margins(body, 24);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), body);
    gtk_window_set_child(GTK_WINDOW(dialog), scroll);
    if (blocked)
      append(body, label(fixture_ ? "SIMULATION · Review only · Terminal "
                                    "continuation is disabled"
                                  : "READ ONLY · Review only · Terminal "
                                    "continuation is disabled",
                         "warning"));
    append(body, label(current_->title.en, "title-2"));
    append(body, label(current_->description.en));
    append(body, label(c::effect_description(plan.effect).en, "warning"));
    append(body, label("Access: " + c::access_name(plan.access) +
                           " · Effect: " + c::effect_name(plan.effect),
                       "caption-heading"));
    append(body, label("Exact validated command", "heading"));
    append(body, text_view(c::display_command(plan.argv), true));
    if (plan.effect == c::Effect::maintenance)
      append(
          body,
          label("This workflow can change storage or prepare reboot. Preserve "
                "backups and a tested external recovery route. The backend "
                "retains its own data-loss and identity confirmations.",
                "error"));
    append(
        body,
        label(
            "Continue in a separate terminal. Existing authorization and "
            "backend confirmations still apply. A launched terminal does not "
            "mean the operation succeeded. Inspect current state afterward."));
    auto required = uke::frontend::required_confirmation(plan);
    GtkWidget *confirmation = nullptr;
    if (!required.empty()) {
      append(body,
             label("Type " + required + " to enable Continue", "heading"));
      confirmation = gtk_entry_new();
      gtk_entry_set_placeholder_text(GTK_ENTRY(confirmation),
                                     "Exact confirmation");
      append(body, confirmation);
    }
    review_phrase_ = confirmation;
    auto *buttons = box(GTK_ORIENTATION_HORIZONTAL, 8);
    append(body, buttons);
    append(buttons, button("Cancel", [dialog] {
             gtk_window_destroy(GTK_WINDOW(dialog));
           }));
    auto *proceed = button(
        "Continue in terminal",
        [this, dialog, plan, required, confirmation] {
          safe([this, dialog, plan, required, confirmation] {
            if (fixture_ || read_only_)
              throw std::runtime_error("Terminal commands are disabled in "
                                       "simulation and read-only mode.");
            if (!required.empty() &&
                required != gtk_editable_get_text(GTK_EDITABLE(confirmation)))
              throw std::runtime_error("Confirmation does not match");
            auto result = uke::frontend::launch_terminal(plan.action_id,
                                                         plan.values, true);
            if (!result.launched)
              throw std::runtime_error(result.error);
            gtk_window_destroy(GTK_WINDOW(dialog));
            gtk_label_set_text(
                GTK_LABEL(result_status_),
                "Continue in terminal; inspect current state afterward. No "
                "operation result has been inferred.");
          });
        },
        plan.effect == c::Effect::maintenance ? "destructive-action"
                                              : "suggested-action");
    append(buttons, proceed);
    review_proceed_ = proceed;
    gtk_widget_set_sensitive(proceed, !blocked && required.empty());
    if (confirmation) {
      auto *pair = new ConfirmationSignal{proceed, required, blocked};
      g_signal_connect_data(
          confirmation, "changed",
          G_CALLBACK(+[](GtkEditable *editable, gpointer data) {
            auto *p = static_cast<ConfirmationSignal *>(data);
            gtk_widget_set_sensitive(
                p->button,
                !p->blocked && p->required == gtk_editable_get_text(editable));
          }),
          pair,
          +[](gpointer data, GClosure *) {
            delete static_cast<ConfirmationSignal *>(data);
          },
          G_CONNECT_DEFAULT);
    }
    gtk_window_present(GTK_WINDOW(dialog));
  }
  gboolean smoke_tick() {
    try {
      if (!fixture_) {
        if (smoke_step_ == 0) {
          open("charge.status");
          start_read();
          ++smoke_step_;
          return G_SOURCE_CONTINUE;
        }
        if (timer_)
          return G_SOURCE_CONTINUE;
        const auto &result = results_.at("charge.status");
        Json::Value data;
        Json::CharReaderBuilder builder;
        std::istringstream input(result.output);
        std::string parse_error;
        if (!Json::parseFromStream(builder, input, &data, &parse_error) ||
            !data["available"].isBool() || data["available"].asBool())
          throw std::runtime_error(
              "Live smoke requires an unavailable charging "
              "provider in an isolated host laboratory");
        if (++smoke_step_ < 20)
          return G_SOURCE_CONTINUE;
        std::cout
            << "GTK live smoke passed: asynchronous installed-client read, "
               "provider explicitly unavailable, no simulation or writes\n";
        gtk_window_close(GTK_WINDOW(window_));
        return G_SOURCE_REMOVE;
      }
      switch (smoke_step_) {
      case 0:
        open("status");
        start_read();
        ++smoke_step_;
        break;
      case 1:
        if (timer_)
          break;
        if (!results_.contains("status") ||
            results_.at("status").output.find("unknown") == std::string::npos)
          throw std::runtime_error("Synthetic status was not rendered");
        open("charge.mode");
        review();
        if (gtk_widget_get_sensitive(review_proceed_))
          throw std::runtime_error("Simulation enabled terminal continuation");
        gtk_window_destroy(GTK_WINDOW(review_window_));
        open("update.apply");
        review();
        if (!review_phrase_)
          throw std::runtime_error("Package review omitted exact confirmation");
        gtk_editable_set_text(GTK_EDITABLE(review_phrase_), "APPLY");
        if (gtk_widget_get_sensitive(review_proceed_))
          throw std::runtime_error(
              "Typed confirmation enabled simulation continuation");
        gtk_window_destroy(GTK_WINDOW(review_window_));
        open("charge.mode");
        if (values().at("mode") != "hold70-80")
          throw std::runtime_error("Choice default differs from client");
        open_related("charge.plan");
        start_read();
        ++smoke_step_;
        break;
      case 2:
        if (timer_)
          break;
        if (results_.at("charge.plan").exit_status != 0)
          throw std::runtime_error("Preview fixture failed");
        go_back();
        if (!current_ || current_->id != "charge.mode")
          throw std::runtime_error("Related-action back navigation failed");
        open("charge.status");
        start_read();
        ++smoke_step_;
        break;
      case 3:
        if (!timer_)
          throw std::runtime_error(
              "Delayed observation did not remain asynchronous");
        cancel(true);
        if (!results_.at("charge.status").cancelled)
          throw std::runtime_error("Cancellation not retained");
        open("overview.audio");
        start_read();
        ++smoke_step_;
        break;
      case 4:
        if (timer_)
          break;
        if (results_.at("overview.audio").exit_status != 69)
          throw std::runtime_error("Missing fixture was not unavailable");
        open("charge.status");
        start_read();
        ++smoke_step_;
        break;
      case 5:
        if (timer_)
          break;
        if (results_.at("charge.status").exit_status != 69 ||
            results_.at("charge.status").error.empty())
          throw std::runtime_error("Fixture provider error lost");
        open("status");
        std::cout << "GTK smoke passed: real widgets, synthetic reads, typed "
                     "choice, preview/back, cancellation, missing data, "
                     "separate provider error, blocked mutations\n";
        gtk_window_close(GTK_WINDOW(window_));
        return G_SOURCE_REMOVE;
      }
      return G_SOURCE_CONTINUE;
    } catch (const std::exception &e) {
      std::cerr << "GTK smoke failed: " << e.what() << '\n';
      exit_code_ = 1;
      gtk_window_close(GTK_WINDOW(window_));
      return G_SOURCE_REMOVE;
    }
  }
};
} // namespace
int main(int argc, char **argv) {
  if (geteuid() == 0) {
    std::cerr << "Run Uke Control GTK as a normal session user. Privileged "
                 "backend actions use the reviewed terminal handoff.\n";
    return 77;
  }
  std::string fixture, start;
  bool readonly = false, smoke = false;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--fixture" && i + 1 < argc)
      fixture = argv[++i];
    else if (arg == "--start" && i + 1 < argc)
      start = argv[++i];
    else if (arg == "--read-only")
      readonly = true;
    else if (arg == "--smoke-test")
      smoke = true;
    else if (arg == "--help") {
      std::cout
          << "Uke Control GTK\nUsage: uke-control-gtk [--fixture FILE] "
             "[--read-only] [--start ACTION]\nCaptured observations are "
             "asynchronous. Changes require review and terminal handoff.\n";
      return 0;
    } else {
      std::cerr << "Unknown or incomplete option: " << arg << '\n';
      return 64;
    }
  }
  try {
    App app(fixture, readonly, smoke, start);
    char *arguments[] = {argv[0], nullptr};
    return app.run(1, arguments);
  } catch (const std::exception &e) {
    std::cerr << "Uke Control GTK: " << e.what() << '\n';
    return 65;
  }
}
