package main

import (
	"fmt"
	"gitconduit-cli/cli/application"
	"strings"
	"time"

	"github.com/rivo/tview"
)

// RepoItem holds a repository with its selected state and current label
type RepoItem struct {
	Repo     application.Repository
	Selected bool
	Label    string
}

// updateLabel updates the Label field of RepoItem based on its properties
func updateLabel(it *RepoItem) {
	label := it.Repo.Name
	if it.Repo.Private {
		label += " [red:{bk}][white:red]Private[red:{bk}]"
	} else {
		label += " [green:{bk}][white:green]Public[green:{bk}]"
	}
	if it.Repo.Fork {
		label += " [yellow:{bk}][white:yellow]Fork[yellow:{bk}]"
	}
	if it.Repo.MirrorUrl != "" {
		label += " [cyan:{bk}][white:cyan]Mirror[cyan:{bk}]"
	}
	it.Label = label + " [-:-:-]"
}

// updateItem updates the displayed text for a single list item at index i
func updateItem(l *tview.List, it RepoItem, i int) {
	checked := "☐ "
	if it.Selected {
		checked = "☑ "
	}
	// determine whether this item is currently highlighted
	current := l.GetCurrentItem()
	var text string
	if i == current {
		text = strings.ReplaceAll(it.Label, "{bk}", "white")
	} else {
		text = strings.ReplaceAll(it.Label, "{bk}", l.GetBackgroundColor().Name())
	}
	l.SetItemText(i, checked+text, "  "+it.Repo.Description)
}

func showAnimatedLoading(app *tview.Application, pages *tview.Pages, prevPage, nextPage string, loadFunc func() error) {
	loading := tview.NewTextView()
	loading.SetTextAlign(tview.AlignCenter)
	loading.SetTitle("Loading").SetTitleAlign(tview.AlignLeft).SetBorder(true)

	pages.AddPage("loading", loading, true, true)
	done := make(chan struct{})

	// Animation goroutine
	go func() {
		frames := []string{"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"}
		i := 0
		for {
			select {
			case <-done:
				return
			default:
				app.QueueUpdateDraw(func() {
					loading.SetText("Please wait " + frames[i%len(frames)])
				})
				i++
				time.Sleep(400 * time.Millisecond)
			}
		}
	}()

	// Simulate loading
	go func() {
		err := loadFunc()
		close(done) // Stop animation
		app.QueueUpdateDraw(func() {
			pages.RemovePage("loading")
			if err != nil {
				// show modal with error and go back to prevPage when dismissed
				modal := tview.NewModal().
					SetText("Error: " + err.Error()).
					AddButtons([]string{"OK"}).SetDoneFunc(func(buttonIndex int, buttonLabel string) {
					pages.RemovePage("errorModal")
					pages.SwitchToPage(prevPage)
				})
				pages.AddPage("errorModal", modal, true, true)
				return
			}
			pages.SwitchToPage(nextPage)
		})
	}()
}

func main() {
	appOptions := []string{"Gogs", "GitBucket", "GitHub", "Bitbucket"}
	sourceApplication := application.NewApplication(application.AppGitHub)
	destApplication := application.NewApplication(application.AppGitHub)

	pages := tview.NewPages()
	app := tview.NewApplication().SetRoot(pages, true)
	authUser := ""

	// Create form1 with controls
	form1Flex := tview.NewFlex().SetDirection(tview.FlexRow)
	form1Flex.SetBorder(true)
	form1 := tview.NewForm()
	sourceAppDropDown := tview.NewDropDown().SetLabel("Application:").
		SetOptions(appOptions, nil).
		SetCurrentOption(0)
	sourceAPIInput := tview.NewInputField().SetLabel("API URL:").SetFieldWidth(50)
	sourceAuthTokenInput := tview.NewInputField().SetLabel("Authorization Token:").SetFieldWidth(50)
	sourceUsernameInput := tview.NewInputField().SetLabel("Username:").SetFieldWidth(50)
	sourcePasswordInput := tview.NewInputField().SetLabel("Password:").SetFieldWidth(50).SetMaskCharacter('*')

	// Create form2 with controls
	form2Flex := tview.NewFlex().SetDirection(tview.FlexRow)
	form2Flex.SetBorder(true)
	form2 := tview.NewForm()
	sourceTypeDropDown := tview.NewDropDown().SetLabel("Type:")
	sourceUserInput := tview.NewInputField().SetLabel("User:        ").SetFieldWidth(50)
	sourceOrgInput := tview.NewInputField().SetLabel("Organization:").SetFieldWidth(50)
	sourceOrgDropDown := tview.NewDropDown().SetLabel("Organization:")

	// Create form3 with controls
	form3Flex := tview.NewFlex().SetDirection(tview.FlexRow)
	form3Flex.SetBorder(true)
	form3 := tview.NewForm()
	var repoItems []RepoItem
	repoList := tview.NewList()
	repoList.SetChangedFunc(func(index int, mainText, secondaryText string, shortcut rune) {
		for i := 0; i < len(repoItems); i++ {
			updateItem(repoList, repoItems[i], i)
		}
	})

	// Create form4 with controls
	form4Flex := tview.NewFlex().SetDirection(tview.FlexRow)
	form4Flex.SetBorder(true)
	form4 := tview.NewForm()
	destAppDropDown := tview.NewDropDown().SetLabel("Application:").
		SetOptions(appOptions, nil).
		SetCurrentOption(0)
	destAPIInput := tview.NewInputField().SetLabel("API URL:").SetFieldWidth(50)
	destAuthTokenInput := tview.NewInputField().SetLabel("Authorization Token:").SetFieldWidth(50)
	destUsernameInput := tview.NewInputField().SetLabel("Username:").SetFieldWidth(50)
	destPasswordInput := tview.NewInputField().SetLabel("Password:").SetFieldWidth(50).SetMaskCharacter('*')

	// Create form5 with controls
	form5Flex := tview.NewFlex().SetDirection(tview.FlexRow)
	form5Flex.SetBorder(true)
	form5 := tview.NewForm()
	destTypeDropDown := tview.NewDropDown().SetLabel("Type:")
	destUserInput := tview.NewInputField().SetLabel("User:        ").SetFieldWidth(50)
	destOrgInput := tview.NewInputField().SetLabel("Organization:").SetFieldWidth(50)
	destOrgDropDown := tview.NewDropDown().SetLabel("Organization:")

	// Create form6 with controls
	form6Flex := tview.NewFlex().SetDirection(tview.FlexRow)
	form6Flex.SetBorder(true)
	form6 := tview.NewForm()
	outputView := tview.NewTextView()
	outputView.SetDynamicColors(true)
	outputView.SetWrap(true)
	outputView.SetScrollable(true)
	outputView.SetBorder(true)
	outputView.SetTitle("Log")
	outputView.SetTitleAlign(tview.AlignLeft)

	beforeForm1 := func() func() {
		return func() {
		}
	}

	beforeForm2 := func(app *tview.Application, prevPage, nextPage string) func() {
		return func() {
			showAnimatedLoading(app, pages, prevPage, nextPage, func() error {
				_, opt := sourceAppDropDown.GetCurrentOption()
				sourceApplication = application.NewApplication(application.ApplicationType(opt))
				sourceApplication.SetApiUrl(sourceAPIInput.GetText())
				sourceApplication.SetToken(sourceAuthTokenInput.GetText())
				sourceApplication.SetUsername(sourceUsernameInput.GetText())
				sourceApplication.SetPassword(sourcePasswordInput.GetText())

				var err error
				authUser, err = sourceApplication.GetAuthenticatedUser()
				if err != nil {
					return fmt.Errorf("GetAuthenticatedUser failed: %w", err)
				}
				sourceUserInput.SetText(authUser)

				orgs, err := sourceApplication.GetOrganizations()
				if err != nil {
					return fmt.Errorf("GetOrganizations failed: %w", err)
				}
				orgNames := make([]string, len(orgs))
				for i, org := range orgs {
					orgNames[i] = org.Name
				}
				switch len(orgs) {
				case 0:
					sourceOrgInput.SetText("")
				case 1:
					sourceOrgInput.SetText(orgNames[0])
				default:
					sourceOrgDropDown.SetOptions(orgNames, nil).SetCurrentOption(0)
				}
				return nil
			})
		}
	}

	beforeForm3 := func(app *tview.Application, prevPage, nextPage string) func() {
		return func() {
			showAnimatedLoading(app, pages, prevPage, nextPage, func() error {
				index, _ := sourceTypeDropDown.GetCurrentOption()
				endpoint := application.EndpointUser
				if index != 0 {
					endpoint = application.EndpointOrganization
				}

				owner := ""
				if endpoint == application.EndpointUser {
					owner = sourceUserInput.GetText()
				} else {
					if sourceOrgDropDown.GetOptionCount() > 1 {
						_, owner = sourceOrgDropDown.GetCurrentOption()
					} else {
						owner = sourceOrgInput.GetText()
					}
				}

				repositories, err := sourceApplication.GetRepositories(endpoint, owner, authUser)
				if err != nil {
					return fmt.Errorf("GetRepositories failed: %w", err)
				}

				repoList.Clear()
				repoItems = make([]RepoItem, 0, len(repositories))
				for i := range repositories {
					// default selected
					item := RepoItem{Repo: repositories[i], Selected: true}
					updateLabel(&item)
					// capture values for closure
					idx := i
					repoItems = append(repoItems, item)
					// add list item with initial text
					repoList.AddItem("", "", 0, func() {
						// toggle selection state
						repoItems[idx].Selected = !repoItems[idx].Selected
						updateItem(repoList, repoItems[idx], idx)
					})
					updateItem(repoList, repoItems[i], i)
				}
				// ensure focus goes to the repo list and first item is selected
				app.QueueUpdateDraw(func() {
					app.SetFocus(repoList)
					if repoList.GetItemCount() > 0 {
						repoList.SetCurrentItem(0)
					}
				})

				return nil
			})
		}
	}

	beforeForm4 := func(_ *tview.Application, _, nextPage string) func() {
		return func() {
			pages.SwitchToPage(nextPage)
		}
	}

	beforeForm5 := func(app *tview.Application, prevPage, nextPage string) func() {
		return func() {
			showAnimatedLoading(app, pages, prevPage, nextPage, func() error {
				_, opt := destAppDropDown.GetCurrentOption()
				destApplication = application.NewApplication(application.ApplicationType(opt))
				destApplication.SetApiUrl(destAPIInput.GetText())
				destApplication.SetToken(destAuthTokenInput.GetText())
				destApplication.SetUsername(destUsernameInput.GetText())
				destApplication.SetPassword(destPasswordInput.GetText())

				var err error
				authUser, err = destApplication.GetAuthenticatedUser()
				if err != nil {
					return fmt.Errorf("GetAuthenticatedUser failed: %w", err)
				}
				destUserInput.SetText(authUser).SetDisabled(true)

				orgs, err := destApplication.GetOrganizations()
				if err != nil {
					return fmt.Errorf("GetOrganizations failed: %w", err)
				}
				orgNames := make([]string, len(orgs))
				for i, org := range orgs {
					orgNames[i] = org.Name
				}
				switch len(orgs) {
				case 0:
					destOrgInput.SetText("")
				case 1:
					destOrgInput.SetText(orgNames[0])
				default:
					destOrgDropDown.SetOptions(orgNames, nil).SetCurrentOption(0)
				}
				return nil
			})
		}
	}

	beforeForm6 := func(app *tview.Application, _, nextPage string) func() {
		return func() {
			outputView.SetText("")
			pages.SwitchToPage(nextPage)
			go func() {
				index, _ := destTypeDropDown.GetCurrentOption()
				endpoint := application.EndpointUser
				if index != 0 {
					endpoint = application.EndpointOrganization
				}

				owner := ""
				if endpoint == application.EndpointUser {
					owner = destUserInput.GetText()
				} else {
					if destOrgDropDown.GetOptionCount() > 1 {
						_, owner = destOrgDropDown.GetCurrentOption()
					} else {
						owner = destOrgInput.GetText()
					}
				}

				for _, item := range repoItems {
					if !item.Selected {
						continue
					}
					repoFullName := item.Repo.FullName
					app.QueueUpdateDraw(func(repoFullName string) func() {
						return func() {
							_, _ = fmt.Fprintf(outputView, "====== %s ======\n", repoFullName)
						}
					}(repoFullName))
					outputView.ScrollToEnd()

					createdRepo, err := destApplication.CreateRepo(endpoint, owner, item.Repo)
					if err != nil {
						app.QueueUpdateDraw(func() {
							_, _ = outputView.Write([]byte("Repository creation failed: " + err.Error() + "\n"))
						})
						outputView.ScrollToEnd()
						continue
					}

					app.QueueUpdateDraw(func() {
						message := fmt.Sprintf("Created repository : %s on %s\n",
							createdRepo.FullName, destApplication.GetApplicationName())
						_, _ = outputView.Write([]byte(message))
					})
					outputView.ScrollToEnd()

					if item.Repo.OpenIssueCount > 0 {
						app.QueueUpdateDraw(func() {
							message := fmt.Sprintf("%d open issue(s) not created!\n", item.Repo.OpenIssueCount)
							_, _ = outputView.Write([]byte(message))
						})
						outputView.ScrollToEnd()
					}

					err = CloneAndPush(item.Repo.CloneUrl, sourceApplication.GetUsername(), sourceApplication.GetPassword(),
						createdRepo.CloneUrl, destApplication.GetUsername(), destApplication.GetPassword())
					message := ""
					if err != nil {
						message = fmt.Sprintf("Error cloning and pushing: %v\n", err)
					} else {
						message = "Cloned and pushed to remote successfully.\n"
					}
					app.QueueUpdateDraw(func() {
						_, _ = outputView.Write([]byte(message))
					})
					outputView.ScrollToEnd()

					if item.Repo.HasWiki {
						sourceWikiUrl := strings.Replace(item.Repo.CloneUrl, ".git", ".wiki.git", 1)
						destWikiUrl := strings.Replace(createdRepo.CloneUrl, ".git", ".wiki.git", 1)

						err = CloneAndPush(sourceWikiUrl, sourceApplication.GetUsername(), sourceApplication.GetPassword(),
							destWikiUrl, destApplication.GetUsername(), destApplication.GetPassword())
						message := ""
						if err != nil {
							message = fmt.Sprintf("Error cloning and pushing wiki: %v\n", err)
						} else {
							message = "Wiki cloned and pushed to remote successfully.\n"
						}
						app.QueueUpdateDraw(func() {
							_, _ = outputView.Write([]byte(message))
						})
						outputView.ScrollToEnd()
					}
				}
				app.QueueUpdateDraw(func() {
					_, _ = outputView.Write([]byte("\n\nCompleted!"))
				})
				outputView.ScrollToEnd()
			}()
		}
	}

	// Setup form1
	form1.
		AddFormItem(sourceAppDropDown).
		AddFormItem(sourceAPIInput).
		AddFormItem(sourceAuthTokenInput).
		AddFormItem(sourceUsernameInput).
		AddFormItem(sourcePasswordInput).
		AddButton("Next", beforeForm2(app, "form1", "form2")).
		AddButton("Quit", func() {
			app.Stop()
		})
	form1Flex.AddItem(tview.NewTextView().SetText("Source").SetTextAlign(tview.AlignCenter), 1, 0, false)
	form1Flex.AddItem(form1, 0, 1, true)
	pages.AddPage("form1", form1Flex, true, true)

	// Setup form2
	form2.AddFormItem(sourceTypeDropDown).
		AddFormItem(sourceUserInput).
		AddButton("Back", func() {
			pages.SwitchToPage("form1")
		}).
		AddButton("Next", beforeForm3(app, "form2", "form3")).
		AddButton("Quit", func() {
			app.Stop()
		})
	form2Flex.AddItem(tview.NewTextView().SetText("Source Owner").SetTextAlign(tview.AlignCenter), 1, 0, false)
	form2Flex.AddItem(form2, 0, 1, true)
	pages.AddPage("form2", form2Flex, true, false)
	sourceTypeDropDown.SetOptions([]string{"User", "Organization"}, func(option string, index int) {
		// Remove both, then add back only the selected one
		form2.Clear(false) // keep buttons intact
		form2.AddFormItem(sourceTypeDropDown)
		if index == 0 {
			form2.AddFormItem(sourceUserInput)
		} else {
			if sourceOrgDropDown.GetOptionCount() > 1 {
				form2.AddFormItem(sourceOrgDropDown)
			} else {
				form2.AddFormItem(sourceOrgInput)
			}
		}
	}).SetCurrentOption(0)

	// Setup form3
	form3.
		AddButton("Back", func() {
			pages.SwitchToPage("form2")
		}).
		AddButton("Next", beforeForm4(app, "form3", "form4")).
		AddButton("Quit", func() {
			app.Stop()
		})
	form3Flex.AddItem(tview.NewTextView().SetText("List of repositories").SetTextAlign(tview.AlignCenter), 1, 0, false)
	form3Flex.AddItem(repoList, 0, 1, true)
	form3Flex.AddItem(form3, 3, 0, false)
	pages.AddPage("form3", form3Flex, true, false)

	// Setup form4
	form4.
		AddFormItem(destAppDropDown).
		AddFormItem(destAPIInput).
		AddFormItem(destAuthTokenInput).
		AddFormItem(destUsernameInput).
		AddFormItem(destPasswordInput).
		AddButton("Back", func() {
			pages.SwitchToPage("form3")
		}).
		AddButton("Next", beforeForm5(app, "form4", "form5")).
		AddButton("Quit", func() {
			app.Stop()
		})
	form4Flex.AddItem(tview.NewTextView().SetText("Destination").SetTextAlign(tview.AlignCenter), 1, 0, false)
	form4Flex.AddItem(form4, 0, 1, true)
	pages.AddPage("form4", form4Flex, true, false)

	// Setup form5
	form5.AddFormItem(destTypeDropDown).
		AddFormItem(destUserInput).
		AddButton("Back", func() {
			pages.SwitchToPage("form4")
		}).
		AddButton("Next", beforeForm6(app, "form5", "form6")).
		AddButton("Quit", func() {
			app.Stop()
		})
	form5Flex.AddItem(tview.NewTextView().SetText("Destination Owner").SetTextAlign(tview.AlignCenter), 1, 0, false)
	form5Flex.AddItem(form5, 0, 1, true)
	pages.AddPage("form5", form5Flex, true, false)
	destTypeDropDown.SetOptions([]string{"User", "Organization"}, func(option string, index int) {
		// Remove both, then add back only the selected one
		form5.Clear(false) // keep buttons intact
		form5.AddFormItem(destTypeDropDown)
		if index == 0 {
			form5.AddFormItem(destUserInput)
		} else {
			if destOrgDropDown.GetOptionCount() > 1 {
				form5.AddFormItem(destOrgDropDown)
			} else {
				form5.AddFormItem(destOrgInput)
			}
		}
	}).SetCurrentOption(0)

	// Setup form6
	form6.
		AddButton("Back", func() {
			pages.SwitchToPage("form5")
		}).
		AddButton("Quit", func() {
			app.Stop()
		})
	form6Flex.AddItem(tview.NewTextView().SetText("Creating repositories").SetTextAlign(tview.AlignCenter), 1, 0, false)
	form6Flex.AddItem(outputView, 0, 1, true)
	form6Flex.AddItem(form6, 3, 0, false)
	pages.AddPage("form6", form6Flex, true, false)

	beforeForm1()()

	if err := app.EnableMouse(true).EnablePaste(true).Run(); err != nil {
		panic(err)
	}
}
