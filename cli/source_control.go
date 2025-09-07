package main

import (
	"fmt"
	"os"
	"strings"

	git "github.com/go-git/go-git/v5"
	"github.com/go-git/go-git/v5/config"
	gitHttp "github.com/go-git/go-git/v5/plumbing/transport/http"
)

func cloneRepository(repo, directory, username, password string) {
	var err error
	cloneOptions := &git.CloneOptions{
		URL: repo,
	}
	if username == "" {
		username = "git" // GitHub requires "git" as the username for PATs
	}
	if password != "" {
		cloneOptions.Auth = &gitHttp.BasicAuth{
			Username: username,
			Password: password,
		}
	}
	_, err = git.PlainClone(directory, true, cloneOptions)
	if err != nil {
		fmt.Printf("Error cloning repository: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("Repository cloned successfully.")
}

func addRemote(directory, repo string) {
	r, err := git.PlainOpen(directory)
	if err != nil {
		fmt.Printf("Error opening repository: %v\n", err)
		os.Exit(1)
	}

	_, err = r.CreateRemote(&config.RemoteConfig{
		Name: "origin2",
		URLs: []string{repo},
	})
	if err != nil {
		fmt.Printf("Error adding remote: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("Remote added successfully.")
}

func pushToRemote(directory, username, password string) {
	r, err := git.PlainOpen(directory)
	if err != nil {
		fmt.Printf("Error opening repository: %v\n", err)
		os.Exit(1)
	}

	pushOptions := &git.PushOptions{
		RemoteName: "origin2",
		RefSpecs: []config.RefSpec{
			config.RefSpec("+refs/*:refs/*"),
		},
		Force: true,
	}
	if username == "" {
		username = "git" // GitHub requires "git" as the username for PATs
	}
	if password != "" {
		pushOptions.Auth = &gitHttp.BasicAuth{
			Username: username,
			Password: password,
		}
	}

	err = r.Push(pushOptions)
	if err != nil {
		fmt.Printf("Error pushing to remote: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("Pushed to remote successfully.")
}

func CloneAndPush(sourcerepo, sourceusername, sourcepassword, destrepo, destusername, destpassword string, hasWiki bool) {
	cnpdirectory := "temp.git"

	defer func() {
		err := os.RemoveAll(cnpdirectory)
		if err != nil {
			fmt.Printf("Warning: failed to remove temp directory %s: %v\n", cnpdirectory, err)
		}
	}()

	cloneRepository(sourcerepo, cnpdirectory, sourceusername, sourcepassword)
	addRemote(cnpdirectory, destrepo)
	pushToRemote(cnpdirectory, destusername, destpassword)

	if hasWiki {
		err := os.RemoveAll(cnpdirectory)
		if err != nil {
			fmt.Printf("Warning: failed to remove temp directory %s: %v\n", cnpdirectory, err)
		}

		sourceWikiUrl := strings.Replace(sourcerepo, ".git", ".wiki.git", 1)
		destWikiUrl := strings.Replace(destrepo, ".git", ".wiki.git", 1)
		cloneRepository(sourceWikiUrl, cnpdirectory, sourceusername, sourcepassword)
		addRemote(cnpdirectory, destWikiUrl)
		pushToRemote(cnpdirectory, destusername, destpassword)
	}
}
