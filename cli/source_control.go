package main

import (
	"fmt"
	"os"

	git "github.com/go-git/go-git/v5"
	"github.com/go-git/go-git/v5/config"
	gitHttp "github.com/go-git/go-git/v5/plumbing/transport/http"
)

// cloneRepository clones a repository to a local directory.
func cloneRepository(repo, directory, username, password string) error {
	if username == "" {
		username = "git" // GitHub requires "git" as the username for PATs
	}

	r, err := git.PlainInit(directory, true)
	if err != nil {
		return fmt.Errorf("error initializing repository: %w", err)
	}

	_, err = r.CreateRemote(&config.RemoteConfig{
		Name: "origin",
		URLs: []string{repo},
		Fetch: []config.RefSpec{
			"+refs/heads/*:refs/heads/*",
			"+refs/tags/*:refs/tags/*",
		},
	})
	if err != nil {
		return fmt.Errorf("error adding remote: %w", err)
	}

	var auth *gitHttp.BasicAuth
	if password != "" {
		auth = &gitHttp.BasicAuth{
			Username: username,
			Password: password,
		}
	}

	err = r.Fetch(&git.FetchOptions{
		RemoteName: "origin",
		RefSpecs: []config.RefSpec{
			"+refs/heads/*:refs/heads/*",
			"+refs/tags/*:refs/tags/*",
		},
		Auth: auth,
	})
	if err != nil {
		return fmt.Errorf("error fetching from remote: %w", err)
	}

	return nil
}

// addRemote adds a new remote to the local repository.
func addRemote(directory, repo string) error {
	r, err := git.PlainOpen(directory)
	if err != nil {
		return fmt.Errorf("error opening repository: %w", err)
	}

	_, err = r.CreateRemote(&config.RemoteConfig{
		Name: "origin2",
		URLs: []string{repo},
	})
	if err != nil {
		return fmt.Errorf("error adding remote: %w", err)
	}
	return nil
}

// pushToRemote pushes the local repository to the specified remote.
func pushToRemote(directory, username, password string) error {
	r, err := git.PlainOpen(directory)
	if err != nil {
		return fmt.Errorf("error opening repository: %w", err)
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
		return fmt.Errorf("error pushing to remote: %w", err)
	}
	return nil
}

// CloneAndPush clones a repository from the source URL and pushes it to the destination URL.
func CloneAndPush(sourcerepo, sourceusername, sourcepassword, destrepo, destusername, destpassword string) error {
	cnpdirectory := "temp.git"

	defer func() {
		err := os.RemoveAll(cnpdirectory)
		if err != nil {
			fmt.Printf("Warning: failed to remove temp directory %s: %v\n", cnpdirectory, err)
		}
	}()

	if err := cloneRepository(sourcerepo, cnpdirectory, sourceusername, sourcepassword); err != nil {
		return fmt.Errorf("clone source repo: %w", err)
	}
	if err := addRemote(cnpdirectory, destrepo); err != nil {
		return fmt.Errorf("add destination remote: %w", err)
	}
	if err := pushToRemote(cnpdirectory, destusername, destpassword); err != nil {
		return fmt.Errorf("push to destination remote: %w", err)
	}

	return nil
}
